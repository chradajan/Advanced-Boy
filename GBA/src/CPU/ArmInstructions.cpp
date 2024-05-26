#include <CPU/ArmInstructions.hpp>
#include <bit>
#include <cmath>
#include <cstdint>
#include <utility>
#include <CPU/ARM7TDMI.hpp>
#include <CPU/CpuTypes.hpp>
#include <Logging/Logging.hpp>
#include <System/InterruptManager.hpp>
#include <System/EventScheduler.hpp>

namespace
{
/// @brief Determine the result of the overflow flag for addition operation: result = op1 + op2
/// @param op1 Addition operand
/// @param op2 Addition operand
/// @param result Addition result
/// @return State of overflow flag after addition
bool AdditionOverflow(uint32_t op1, uint32_t op2, uint32_t result)
{
    return (~(op1 ^ op2) & ((op1 ^ result)) & 0x8000'0000) != 0;
}

/// @brief Determine the result of the overflow flag for subtraction operation: result = op1 - op2
/// @param op1 Subtraction operand
/// @param op2 Subtraction operand
/// @param result Subtraction result
/// @return State of overflow flag after subtraction
bool SubtractionOverflow(uint32_t op1, uint32_t op2, uint32_t result)
{
    return ((op1 ^ op2) & ((op1 ^ result)) & 0x8000'0000) != 0;
}

/// @brief Calculate the result of a THUMB add or add w/ carry operation, along with the carry and overflow flags.
/// @param op1 First addition operand.
/// @param op2 Second addition operand.
/// @param result Result of addition, returned by reference.
/// @param carry Carry flag, defaults to 0 meaning non ADC operation.
/// @return Pair of {carry_flag, overflow_flag}.
std::pair<bool, bool> Add32(uint32_t op1, uint32_t op2, uint32_t& result, bool carry = 0)
{
    uint64_t result64 = static_cast<uint64_t>(op1) + static_cast<uint64_t>(op2) + static_cast<uint64_t>(carry);
    result = result64 & MAX_U32;
    bool c = (result64 > MAX_U32);
    bool v = AdditionOverflow(op1, op2, result);
    return {c, v};
}

/// @brief Calculate the result of a THUMB sub or sub w/ carry operation, along with the carry and overflow flags.
/// @param op1 First subtraction operand.
/// @param op2 Second subtraction operand.
/// @param result Result of subtraction (op1 - op2), returned by reference.
/// @param sbc Whether this is a subtract with carry operation.
/// @param carry Current value of carry flag.
/// @return Pair of {carry_flag, overflow_flag}.
std::pair<bool, bool> Sub32(uint32_t op1, uint32_t op2, uint32_t& result, bool sbc = false, bool carry = 0)
{
    uint64_t result64;
    bool c;

    if (sbc)
    {
        result64 = static_cast<uint64_t>(op1) + static_cast<uint64_t>(~op2) + static_cast<uint64_t>(carry);
        c = (result64 > MAX_U32);
    }
    else
    {
        result64 = static_cast<uint64_t>(op1) + static_cast<uint64_t>(~op2 + 1);
        c = op1 >= op2;
    }

    result = result64 & MAX_U32;
    bool v = SubtractionOverflow(op1, op2, result);
    return {c, v};
}
}

namespace CPU::ARM
{
using namespace Logging;

CPU::Instruction* DecodeInstruction(uint32_t undecodedInstruction, void* buffer)
{
    CPU::Instruction* instructionPtr = nullptr;

    if (BranchAndExchange::IsInstanceOf(undecodedInstruction))
    {
        instructionPtr = new(buffer) BranchAndExchange(undecodedInstruction);
    }
    else if (BlockDataTransfer::IsInstanceOf(undecodedInstruction))
    {
        instructionPtr = new(buffer) BlockDataTransfer(undecodedInstruction);
    }
    else if (Branch::IsInstanceOf(undecodedInstruction))
    {
        instructionPtr = new(buffer) Branch(undecodedInstruction);
    }
    else if (SoftwareInterrupt::IsInstanceOf(undecodedInstruction))
    {
        instructionPtr = new(buffer) SoftwareInterrupt(undecodedInstruction);
    }
    else if (Undefined::IsInstanceOf(undecodedInstruction))
    {
        instructionPtr = new(buffer) Undefined(undecodedInstruction);
    }
    else if (SingleDataTransfer::IsInstanceOf(undecodedInstruction))
    {
        instructionPtr = new(buffer) SingleDataTransfer(undecodedInstruction);
    }
    else if (SingleDataSwap::IsInstanceOf(undecodedInstruction))
    {
        instructionPtr = new(buffer) SingleDataSwap(undecodedInstruction);
    }
    else if (Multiply::IsInstanceOf(undecodedInstruction))
    {
        instructionPtr = new(buffer) Multiply(undecodedInstruction);
    }
    else if (MultiplyLong::IsInstanceOf(undecodedInstruction))
    {
        instructionPtr = new(buffer) MultiplyLong(undecodedInstruction);
    }
    else if (HalfwordDataTransferRegisterOffset::IsInstanceOf(undecodedInstruction))
    {
        instructionPtr = new(buffer) HalfwordDataTransferRegisterOffset(undecodedInstruction);
    }
    else if (HalfwordDataTransferImmediateOffset::IsInstanceOf(undecodedInstruction))
    {
        instructionPtr = new(buffer) HalfwordDataTransferImmediateOffset(undecodedInstruction);
    }
    else if (PSRTransferMRS::IsInstanceOf(undecodedInstruction))
    {
        instructionPtr = new(buffer) PSRTransferMRS(undecodedInstruction);
    }
    else if (PSRTransferMSR::IsInstanceOf(undecodedInstruction))
    {
        instructionPtr = new(buffer) PSRTransferMSR(undecodedInstruction);
    }
    else if (DataProcessing::IsInstanceOf(undecodedInstruction))
    {
        instructionPtr = new(buffer) DataProcessing(undecodedInstruction);
    }

    return instructionPtr;
}

void BranchAndExchange::Execute(ARM7TDMI& cpu)
{
    if (LogMgr.LoggingEnabled())
    {
        SetMnemonic(cpu.mnemonic_);
    }

    if (!cpu.ArmConditionSatisfied(instruction_.Cond))
    {
        return;
    }

    uint32_t newPC = cpu.registers_.ReadRegister(instruction_.Rn);

    if (newPC & 0x01)
    {
        cpu.registers_.SetOperatingState(OperatingState::THUMB);
        newPC &= 0xFFFF'FFFE;
    }
    else
    {
        cpu.registers_.SetOperatingState(OperatingState::ARM);
        newPC &= 0xFFFF'FFFC;
    }

    cpu.registers_.SetPC(newPC);
    cpu.flushPipeline_ = true;
}

void BlockDataTransfer::Execute(ARM7TDMI& cpu)
{
    if (LogMgr.LoggingEnabled())
    {
        SetMnemonic(cpu.mnemonic_);
    }

    if (!cpu.ArmConditionSatisfied(instruction_.Cond))
    {
        return;
    }

    uint16_t regList = instruction_.RegisterList;
    bool emptyRlist = (regList == 0);
    bool wbIndexInList = ((instruction_.W) && (regList & (0x01 << instruction_.Rn)));
    bool wbIndexFirstInList = true;

    if (wbIndexInList && !instruction_.L)
    {
        for (uint16_t i = 0x01; i < (0x01 << instruction_.Rn); i <<= 1)
        {
            if (i & regList)
            {
                wbIndexFirstInList = false;
                break;
            }
        }
    }

    if (!regList)
    {
        // If regList is empty, R15 is still stored/loaded
        regList = 0x8000;
    }

    OperatingMode mode = cpu.registers_.GetOperatingMode();
    bool r15InList = instruction_.RegisterList & 0x8000;

    if (instruction_.S)
    {
        if (r15InList)
        {
            if (!instruction_.L)
            {
                mode = OperatingMode::User;
            }
        }
        else
        {
            mode = OperatingMode::User;
        }
    }

    int regListSize = std::popcount(regList);
    uint32_t baseAddr = cpu.registers_.ReadRegister(instruction_.Rn);
    uint32_t minAddr;
    uint32_t wbAddr;
    bool preIndexOffset = instruction_.P;

    if (instruction_.U)
    {
        minAddr = preIndexOffset ? (baseAddr + 4) : baseAddr;

        if (preIndexOffset)
        {
            wbAddr = minAddr + (4 * (regListSize - 1));
        }
        else
        {
            wbAddr = minAddr + (4 * regListSize);
        }

        if (emptyRlist)
        {
            wbAddr = baseAddr + 0x40;
        }
    }
    else
    {
        if (preIndexOffset)
        {
            minAddr = baseAddr - (4 * regListSize);
            wbAddr = minAddr;
        }
        else
        {
            minAddr = baseAddr - (4 * (regListSize - 1));
            wbAddr = minAddr - 4;
        }

        if (emptyRlist)
        {
            minAddr = baseAddr - (preIndexOffset ? 0x40 : 0x3C);
            wbAddr = baseAddr - 0x40;
        }
    }

    uint8_t regIndex = 0;
    uint32_t addr = minAddr;

    while (regList != 0)
    {
        if (regList & 0x01)
        {
            if (instruction_.L)
            {
                auto [readValue, readCycles] = cpu.ReadMemory(addr, AccessSize::WORD);
                Scheduler.Step(readCycles);

                if (regIndex == PC_INDEX)
                {
                    cpu.flushPipeline_ = true;

                    if (instruction_.S)
                    {
                        cpu.registers_.LoadSPSR();
                        InterruptMgr.CheckForInterrupt();
                    }
                }

                cpu.registers_.WriteRegister(regIndex, readValue, mode);
            }
            else
            {
                uint32_t regValue = cpu.registers_.ReadRegister(regIndex, mode);

                if (regIndex == PC_INDEX)
                {
                    regValue += 4;
                }
                else if ((regIndex == instruction_.Rn) && !wbIndexFirstInList)
                {
                    regValue = wbAddr;
                }

                int writeCycles = cpu.WriteMemory(addr, regValue, AccessSize::WORD);
                Scheduler.Step(writeCycles);
            }

            addr += 4;
        }

        ++regIndex;
        regList >>= 1;
    }

    if (instruction_.W && !(wbIndexInList && instruction_.L))
    {
        cpu.registers_.WriteRegister(instruction_.Rn, wbAddr);
    }

    if (instruction_.L)
    {
        // Run 1 extra internal cycle for final write back of load operation.
        Scheduler.Step(1);
    }
}

void Branch::Execute(ARM7TDMI& cpu)
{
    int32_t signedOffset = instruction_.Offset << 2;

    if (signedOffset & 0x0200'0000)
    {
        signedOffset |= 0xFC00'0000;
    }

    uint32_t newPC = cpu.registers_.GetPC() + signedOffset;

    if (LogMgr.LoggingEnabled())
    {
        SetMnemonic(cpu.mnemonic_, newPC);
    }

    if (!cpu.ArmConditionSatisfied(instruction_.Cond))
    {
        return;
    }

    if (instruction_.L)
    {
        cpu.registers_.WriteRegister(14, (cpu.registers_.GetPC() - 4) & 0xFFFF'FFFC);
    }

    cpu.registers_.SetPC(newPC);
    cpu.flushPipeline_ = true;
}

void SoftwareInterrupt::Execute(ARM7TDMI& cpu)
{
    if (LogMgr.LoggingEnabled())
    {
        SetMnemonic(cpu.mnemonic_);
    }

    if (!cpu.ArmConditionSatisfied(instruction_.Cond))
    {
        return;
    }

    uint32_t currentCPSR = cpu.registers_.GetCPSR();
    cpu.registers_.SetOperatingMode(OperatingMode::Supervisor);
    cpu.registers_.WriteRegister(LR_INDEX, cpu.registers_.GetPC() - 4);
    cpu.registers_.SetIrqDisabled(true);
    cpu.registers_.SetSPSR(currentCPSR);
    cpu.registers_.SetPC(0x0000'0008);
    cpu.flushPipeline_ = true;
}

void Undefined::Execute(ARM7TDMI& cpu)
{
    if (LogMgr.LoggingEnabled())
    {
        SetMnemonic(cpu.mnemonic_);
    }

    if (!cpu.ArmConditionSatisfied(instruction_.Cond))
    {
        return;
    }

    uint32_t currentCPSR = cpu.registers_.GetCPSR();
    cpu.registers_.SetOperatingMode(OperatingMode::Undefined);
    cpu.registers_.WriteRegister(LR_INDEX, cpu.registers_.GetPC() - 4);
    cpu.registers_.SetIrqDisabled(true);
    cpu.registers_.SetSPSR(currentCPSR);
    cpu.registers_.SetPC(0x0000'0004);
    cpu.flushPipeline_ = true;
}

void SingleDataTransfer::Execute(ARM7TDMI& cpu)
{
    uint8_t baseIndex = instruction_.flags.Rn;
    uint8_t srcDestIndex = instruction_.flags.Rd;
    uint32_t offset;

    if (!instruction_.flags.I)
    {
        offset = instruction_.flags.Offset;
    }
    else
    {
        uint8_t shiftAmount = instruction_.registerOffset.ShiftAmount;
        offset = cpu.registers_.ReadRegister(instruction_.registerOffset.Rm);

        switch (instruction_.registerOffset.ShiftType)
        {
            case 0b00:  // LSL
                offset <<= shiftAmount;
                break;
            case 0b01:  // LSR
                offset = (shiftAmount == 0) ? 0 : (offset >> shiftAmount);
                break;
            case 0b10:  // ASR
            {
                bool msbSet = offset & 0x8000'0000;

                if (shiftAmount == 0)
                {
                    offset = msbSet ? 0xFFFF'FFFF : 0;
                }
                else
                {
                    for (int i = 0; i < shiftAmount; ++i)
                    {
                        offset >>= 1;
                        offset |= (msbSet ? 0x8000'0000 : 0);
                    }
                }

                break;
            }
            case 0b11:  // ROR, RRX
            {
                if (shiftAmount == 0)
                {
                    offset >>= 1;
                    offset |= (cpu.registers_.IsCarry() ? 0x8000'0000 : 0);
                }
                else
                {
                    offset = std::rotr(offset, shiftAmount);
                }
                break;
            }
        }
    }

    if (LogMgr.LoggingEnabled())
    {
        SetMnemonic(cpu.mnemonic_, offset);
    }

    if (!cpu.ArmConditionSatisfied(instruction_.flags.Cond))
    {
        return;
    }

    int32_t signedOffset = (instruction_.flags.U ? offset : -offset);
    uint32_t addr = cpu.registers_.ReadRegister(baseIndex);
    bool preIndex = instruction_.flags.P;
    bool postIndex = !preIndex;
    bool ignoreWriteback = false;

    if (preIndex)
    {
        addr += signedOffset;
    }

    if (instruction_.flags.L)
    {
        // Load
        AccessSize alignment = instruction_.flags.B ? AccessSize::BYTE : AccessSize::WORD;
        auto [value, readCycles] = cpu.ReadMemory(addr, alignment);
        Scheduler.Step(readCycles);

        if ((alignment == AccessSize::WORD) && (addr & 0x03))
        {
            value = std::rotr(value, (addr & 0x03) * 8);
        }

        cpu.registers_.WriteRegister(srcDestIndex, value);

        cpu.flushPipeline_ = (srcDestIndex == PC_INDEX);
        ignoreWriteback = (srcDestIndex == baseIndex);
    }
    else
    {
        // Store
        uint32_t value = cpu.registers_.ReadRegister(srcDestIndex);
        AccessSize alignment = AccessSize::WORD;

        if (srcDestIndex == PC_INDEX)
        {
            value += 4;
        }

        if (instruction_.flags.B)
        {
            value &= MAX_U8;
            alignment = AccessSize::BYTE;
        }

        int writeCycles = cpu.WriteMemory(addr, value, alignment);
        Scheduler.Step(writeCycles);
    }

    if (postIndex)
    {
        addr += signedOffset;
    }

    if (!ignoreWriteback && (instruction_.flags.W || postIndex))
    {
        cpu.registers_.WriteRegister(baseIndex, addr);
    }

    if (instruction_.flags.L)
    {
        // Run 1 extra internal cycle for final write back of load operation.
        Scheduler.Step(1);
    }
}

void SingleDataSwap::Execute(ARM7TDMI& cpu)
{
    if (LogMgr.LoggingEnabled())
    {
        SetMnemonic(cpu.mnemonic_);
    }

    if (!cpu.ArmConditionSatisfied(instruction_.Cond))
    {
        return;
    }

    uint32_t addr = cpu.registers_.ReadRegister(instruction_.Rn);
    AccessSize alignment = instruction_.B ? AccessSize::BYTE : AccessSize::WORD;

    auto [memValue, readCycles] = cpu.ReadMemory(addr, alignment);
    uint32_t regValue = cpu.registers_.ReadRegister(instruction_.Rm);

    if ((alignment == AccessSize::WORD) && (addr & 0x03))
    {
        memValue = std::rotr(memValue, (addr & 0x03) * 8);
    }

    int writeCycles = cpu.WriteMemory(addr, regValue, alignment);
    cpu.registers_.WriteRegister(instruction_.Rd, memValue);

    Scheduler.Step(readCycles + writeCycles);
}

void Multiply::Execute(ARM7TDMI& cpu)
{
    if (LogMgr.LoggingEnabled())
    {
        SetMnemonic(cpu.mnemonic_);
    }

    if (!cpu.ArmConditionSatisfied(instruction_.Cond))
    {
        return;
    }

    uint8_t destIndex = instruction_.Rd;
    uint32_t rm = cpu.registers_.ReadRegister(instruction_.Rm);
    uint32_t rs = cpu.registers_.ReadRegister(instruction_.Rs);
    uint32_t rn = cpu.registers_.ReadRegister(instruction_.Rn);
    int64_t result;

    int cycles;

    if (((rs & 0xFFFF'FF00) == 0xFFFF'FF00) || ((rs & 0xFFFF'FF00) == 0))
    {
        cycles = 1;
    }
    else if (((rs & 0xFFFF'0000) == 0xFFFF'0000) || ((rs & 0xFFFF'0000) == 0))
    {
        cycles = 2;
    }
    else if (((rs & 0xFF00'0000) == 0xFF00'0000) || ((rs & 0xFF00'0000) == 0))
    {
        cycles = 3;
    }
    else
    {
        cycles = 4;
    }

    if (instruction_.A)
    {
        // MLA
        result = (rm * rs) + rn;
        ++cycles;
    }
    else
    {
        // MUL
        result = rm * rs;
    }

    if (instruction_.S)
    {
        cpu.registers_.SetNegative(result & 0x8000'0000);
        cpu.registers_.SetZero(result == 0);
    }

    cpu.registers_.WriteRegister(destIndex, result & MAX_U32);
    Scheduler.Step(cycles);
}

void MultiplyLong::Execute(ARM7TDMI& cpu)
{
    if (LogMgr.LoggingEnabled())
    {
        SetMnemonic(cpu.mnemonic_);
    }

    if (!cpu.ArmConditionSatisfied(instruction_.Cond))
    {
        return;
    }

    uint32_t rm = cpu.registers_.ReadRegister(instruction_.Rm);
    uint32_t rs = cpu.registers_.ReadRegister(instruction_.Rs);
    uint32_t rdHi = cpu.registers_.ReadRegister(instruction_.RdHi);
    uint32_t rdLo = cpu.registers_.ReadRegister(instruction_.RdLo);
    uint64_t rdHiLo = (static_cast<uint64_t>(rdHi) << 32) | rdLo;

    uint64_t result;
    int cycles = instruction_.A ? 2 : 1;

    if (instruction_.U)
    {
        // Signed
        if (((rs & 0xFFFF'FF00) == 0xFFFF'FF00) || ((rs & 0xFFFF'FF00) == 0))
        {
            cycles += 1;
        }
        else if (((rs & 0xFFFF'0000) == 0xFFFF'0000) || ((rs & 0xFFFF'0000) == 0))
        {
            cycles += 2;
        }
        else if (((rs & 0xFF00'0000) == 0xFF00'0000) || ((rs & 0xFF00'0000) == 0))
        {
            cycles += 3;
        }
        else
        {
            cycles += 4;
        }

        int64_t op1 = rm;
        int64_t op2 = rs;
        int64_t op3 = rdHiLo;

        if (op1 & 0x8000'0000)
        {
            op1 |= 0xFFFF'FFFF'0000'0000;
        }

        if (op2 & 0x8000'0000)
        {
            op2 |= 0xFFFF'FFFF'0000'0000;
        }

        int64_t signedResult = instruction_.A ? ((op1 * op2) + op3) : (op1 * op2);
        result = static_cast<uint64_t>(signedResult);
    }
    else
    {
        // Unsigned
        if ((rs & 0xFFFF'FF00) == 0)
        {
            cycles += 1;
        }
        else if ((rs & 0xFFFF'0000) == 0)
        {
            cycles += 2;
        }
        else if ((rs & 0xFF00'0000) == 0)
        {
            cycles += 3;
        }
        else
        {
            cycles += 4;
        }

        uint64_t op1 = rm;
        uint64_t op2 = rs;
        uint64_t op3 = rdHiLo;
        result = instruction_.A ? ((op1 * op2) + op3) : (op1 * op2);
    }

    if (instruction_.S)
    {
        cpu.registers_.SetNegative(result & 0x8000'0000'0000'0000);
        cpu.registers_.SetZero(result == 0);
    }

    cpu.registers_.WriteRegister(instruction_.RdHi, result >> 32);
    cpu.registers_.WriteRegister(instruction_.RdLo, result & MAX_U32);
    Scheduler.Step(cycles);
}

void HalfwordDataTransferRegisterOffset::Execute(ARM7TDMI& cpu)
{
    uint32_t unsignedOffset = cpu.registers_.ReadRegister(instruction_.Rm);
    int16_t signedOffset = instruction_.U ? unsignedOffset : -unsignedOffset;
    bool preIndex = instruction_.P;
    bool postIndex = !preIndex;
    bool ignoreWriteback = false;
    uint8_t baseIndex = instruction_.Rn;
    uint8_t srcDestIndex = instruction_.Rd;
    uint32_t addr = cpu.registers_.ReadRegister(baseIndex);

    if (LogMgr.LoggingEnabled())
    {
        SetMnemonic(cpu.mnemonic_, unsignedOffset);
    }

    if (!cpu.ArmConditionSatisfied(instruction_.Cond))
    {
        return;
    }

    if (preIndex)
    {
        addr += signedOffset;
    }

    if (instruction_.L)  // Load
    {
        bool s = instruction_.S;
        bool h = instruction_.H;
        bool misaligned = addr & 0x01;
        cpu.flushPipeline_ = (srcDestIndex == PC_INDEX);
        ignoreWriteback = (srcDestIndex == baseIndex);

        // LDRH Rd,[odd]   -->  LDRH Rd,[odd-1] ROR 8
        // LDRSH Rd,[odd]  -->  LDRSB Rd,[odd]

        if (s)
        {
            uint32_t signExtendedWord;

            if (misaligned && h)
            {
                // Convert LDRSH into LDRSB
                h = false;
            }

            if (h)
            {
                // S = 1, H = 1
                auto [halfWord, readCycles] = cpu.ReadMemory(addr, AccessSize::HALFWORD);
                Scheduler.Step(readCycles);
                signExtendedWord = halfWord;

                if (halfWord & 0x8000)
                {
                    signExtendedWord |= 0xFFFF'0000;
                }

                cpu.registers_.WriteRegister(srcDestIndex, signExtendedWord);
            }
            else
            {
                // S = 1, H = 0
                auto [byte, readCycles] = cpu.ReadMemory(addr, AccessSize::BYTE);
                Scheduler.Step(readCycles);

                signExtendedWord = byte;

                if (byte & 0x80)
                {
                    signExtendedWord |= 0xFFFF'FF00;
                }

                cpu.registers_.WriteRegister(srcDestIndex, signExtendedWord);
            }
        }
        else
        {
            // S = 0, H = 1
            auto [halfWord, readCycles] = cpu.ReadMemory(addr, AccessSize::HALFWORD);
            Scheduler.Step(readCycles);

            if (misaligned)
            {
                halfWord = std::rotr(halfWord, 8);
            }

            cpu.registers_.WriteRegister(srcDestIndex, halfWord);
        }
    }
    else  // Store
    {
        // S = 0, H = 1
        uint16_t halfWord = cpu.registers_.ReadRegister(srcDestIndex);

        if (srcDestIndex == PC_INDEX)
        {
            halfWord += 4;
        }

        int writeCycles = cpu.WriteMemory(addr, halfWord, AccessSize::HALFWORD);
        Scheduler.Step(writeCycles);
    }

    if (postIndex)
    {
        addr += signedOffset;
    }

    if (!ignoreWriteback && (instruction_.W || postIndex))
    {
        cpu.registers_.WriteRegister(baseIndex, addr);
    }

    if (instruction_.L)
    {
        // Run 1 extra internal cycle for final write back of load operation.
        Scheduler.Step(1);
    }
}

void HalfwordDataTransferImmediateOffset::Execute(ARM7TDMI& cpu)
{
    uint8_t unsignedOffset = (instruction_.Offset1 << 4) | instruction_.Offset;
    int16_t signedOffset = instruction_.U ? unsignedOffset : -unsignedOffset;
    bool preIndex = instruction_.P;
    bool postIndex = !preIndex;
    bool ignoreWriteback = false;
    uint8_t baseIndex = instruction_.Rn;
    uint8_t srcDestIndex = instruction_.Rd;
    uint32_t addr = cpu.registers_.ReadRegister(baseIndex);

    if (LogMgr.LoggingEnabled())
    {
        SetMnemonic(cpu.mnemonic_, unsignedOffset);
    }

    if (!cpu.ArmConditionSatisfied(instruction_.Cond))
    {
        return;
    }

    if (preIndex)
    {
        addr += signedOffset;
    }

    if (instruction_.L)  // Load
    {
        bool s = instruction_.S;
        bool h = instruction_.H;
        bool misaligned = addr & 0x01;
        cpu.flushPipeline_ = (srcDestIndex == PC_INDEX);
        ignoreWriteback = (srcDestIndex == baseIndex);

        // LDRH Rd,[odd]   -->  LDRH Rd,[odd-1] ROR 8
        // LDRSH Rd,[odd]  -->  LDRSB Rd,[odd]

        if (s)
        {
            uint32_t signExtendedWord;

            if (misaligned && h)
            {
                // Convert LDRSH into LDRSB
                h = false;
            }

            if (h)
            {
                // S = 1, H = 1
                auto [halfWord, readCycles] = cpu.ReadMemory(addr, AccessSize::HALFWORD);
                Scheduler.Step(readCycles);
                signExtendedWord = halfWord;

                if (halfWord & 0x8000)
                {
                    signExtendedWord |= 0xFFFF'0000;
                }

                cpu.registers_.WriteRegister(srcDestIndex, signExtendedWord);
            }
            else
            {
                // S = 1, H = 0
                auto [byte, readCycles] = cpu.ReadMemory(addr, AccessSize::BYTE);
                Scheduler.Step(readCycles);
                signExtendedWord = byte;

                if (byte & 0x80)
                {
                    signExtendedWord |= 0xFFFF'FF00;
                }

                cpu.registers_.WriteRegister(srcDestIndex, signExtendedWord);
            }
        }
        else
        {
            // S = 0, H = 1
            auto [halfWord, readCycles] = cpu.ReadMemory(addr, AccessSize::HALFWORD);
            Scheduler.Step(readCycles);

            if (misaligned)
            {
                halfWord = std::rotr(halfWord, 8);
            }

            cpu.registers_.WriteRegister(srcDestIndex, halfWord);
        }
    }
    else  // Store
    {
        // S = 0, H = 1
        uint16_t halfWord = cpu.registers_.ReadRegister(srcDestIndex);

        if (srcDestIndex == PC_INDEX)
        {
            halfWord += 4;
        }

        int writeCycles = cpu.WriteMemory(addr, halfWord, AccessSize::HALFWORD);
        Scheduler.Step(writeCycles);
    }

    if (postIndex)
    {
        addr += signedOffset;
    }

    if (!ignoreWriteback && (instruction_.W || postIndex))
    {
        cpu.registers_.WriteRegister(baseIndex, addr);
    }

    if (instruction_.L)
    {
        // Run 1 extra internal cycle for final write back of load operation.
        Scheduler.Step(1);
    }
}

void PSRTransferMRS::Execute(ARM7TDMI& cpu)
{
    if (LogMgr.LoggingEnabled())
    {
        SetMnemonic(cpu.mnemonic_);
    }

    if (!cpu.ArmConditionSatisfied(instruction_.Cond))
    {
        return;
    }

    uint32_t value = instruction_.Ps ? cpu.registers_.GetSPSR() : cpu.registers_.GetCPSR();
    cpu.registers_.WriteRegister(instruction_.Rd, value);
}

void PSRTransferMSR::Execute(ARM7TDMI& cpu)
{
    if (LogMgr.LoggingEnabled())
    {
        SetMnemonic(cpu.mnemonic_);
    }

    if (!cpu.ArmConditionSatisfied(instruction_.commonFlags.Cond))
    {
        return;
    }

    uint32_t value;

    if (instruction_.commonFlags.I)
    {
        value = instruction_.immFlag.Imm;
        value = std::rotr(value, instruction_.immFlag.Rotate * 2);
    }
    else
    {
        value = cpu.registers_.ReadRegister(instruction_.regFlags.Rm);
    }

    uint32_t mask = 0;
    mask |= instruction_.commonFlags.Flags ? 0xFF00'0000 : 0;

    if (cpu.registers_.GetOperatingMode() != OperatingMode::User)
    {
        mask |= instruction_.commonFlags.Status ? 0x00FF'0000 : 0;
        mask |= instruction_.commonFlags.Extension ? 0x0000'FF00 : 0;
        mask |= instruction_.commonFlags.Control ? 0x0000'00FF : 0;
    }

    if (mask == 0)
    {
        return;
    }

    value &= mask;

    if (instruction_.commonFlags.Pd)
    {
        // Update SPSR
        uint32_t spsr = cpu.registers_.GetSPSR();
        spsr &= ~mask;
        spsr |= value;
        cpu.registers_.SetSPSR(spsr);
    }
    else
    {
        // Update CPSR
        uint32_t cpsr = cpu.registers_.GetCPSR();
        cpsr &= ~mask;
        cpsr |= value;
        cpu.registers_.SetCPSR(cpsr);
        InterruptMgr.CheckForInterrupt();
    }
}

void DataProcessing::Execute(ARM7TDMI& cpu)
{
    uint32_t op1 = cpu.registers_.ReadRegister(instruction_.flags.Rn);
    uint32_t op2;
    uint8_t destIndex = instruction_.flags.Rd;

    bool carryOut = cpu.registers_.IsCarry();
    bool overflowOut = cpu.registers_.IsOverflow();

    if (instruction_.flags.I)
    {
        // Operand 2 is an immediate value
        op2 = instruction_.rotatedImmediate.Imm;
        uint8_t rotateAmount = instruction_.rotatedImmediate.RotateAmount << 1;
        carryOut = (op2 & (0x01 << (rotateAmount - 1)));
        op2 = std::rotr(op2, rotateAmount);
    }
    else
    {
        // Operand 2 is a register
        op2 = cpu.registers_.ReadRegister(instruction_.shiftRegByReg.Rm);
        bool shiftByReg = (instruction_.flags.Operand2 & 0x10);

        uint8_t shiftAmount =
            shiftByReg ? (cpu.registers_.ReadRegister(instruction_.shiftRegByReg.Rs) & 0xFF) :
            instruction_.shiftRegByImm.ShiftAmount;

        if (shiftByReg)
        {
            // If R15 is used as an operand and a register specified shift amount is used, PC will be 12 bytes ahead.
            if (instruction_.flags.Rn == PC_INDEX)
            {
                op1 += 4;
            }

            if (instruction_.shiftRegByReg.Rm == PC_INDEX)
            {
                op2 += 4;
            }

            // One extra internal cycles when shifting by register.
            Scheduler.Step(1);
        }

        switch (instruction_.shiftRegByReg.ShiftType)
        {
            case 0b00:  // LSL
            {
                if (shiftAmount > 32)
                {
                    carryOut = false;
                    op2 = 0;
                }
                else if (shiftAmount == 32)
                {
                    carryOut = (op2 & 0x01);
                    op2 = 0;
                }
                else if (shiftAmount > 0)
                {
                    carryOut = (op2 & (0x8000'0000 >> (shiftAmount - 1)));
                    op2 <<= shiftAmount;
                }

                break;
            }
            case 0b01:  // LSR
            {
                if (shiftAmount > 32)
                {
                    carryOut = false;
                    op2 = 0;
                }
                else if (shiftAmount == 32)
                {
                    carryOut = (op2 & 0x8000'0000);
                    op2 = 0;
                }
                else if (shiftAmount > 0)
                {
                    carryOut = (op2 & (0x01 << (shiftAmount - 1)));
                    op2 >>= shiftAmount;
                }
                else if (!shiftByReg)
                {
                    // LSR #0 -> LSR #32
                    carryOut = (op2 & 0x8000'0000);
                    op2 = 0;
                }

                break;
            }
            case 0b10:  // ASR
            {
                bool msbSet = op2 & 0x8000'0000;

                if (shiftAmount >= 32)
                {
                    carryOut = msbSet;
                    op2 = msbSet ? 0xFFFF'FFFF : 0;
                }
                else if (shiftAmount > 0)
                {
                    carryOut = (op2 & (0x01 << (shiftAmount - 1)));

                    for (int i = 0; i < shiftAmount; ++i)
                    {
                        op2 >>= 1;
                        op2 |= (msbSet ? 0x8000'0000 : 0);
                    }
                }
                else if (!shiftByReg)
                {
                    // ASR #0 -> ASR #32
                    carryOut = msbSet;
                    op2 = msbSet ? 0xFFFF'FFFF : 0;
                }

                break;
            }
            case 0b11:  // ROR, RRX
            {
                if (shiftAmount > 32)
                {
                    shiftAmount %= 32;
                }

                if (shiftAmount == 0)
                {
                    if (!shiftByReg)
                    {
                        // ROR #0 -> RRX
                        carryOut = op2 & 0x01;
                        op2 >>= 1;
                        op2 |= cpu.registers_.IsCarry() ? 0x8000'0000 : 0;
                    }
                }
                else
                {
                    carryOut = (op2 & (0x01 << (shiftAmount - 1)));
                    op2 = std::rotr(op2, shiftAmount);
                }
                break;
            }
        }
    }

    if (LogMgr.LoggingEnabled())
    {
        SetMnemonic(cpu.mnemonic_, op2);
    }

    if (!cpu.ArmConditionSatisfied(instruction_.flags.Cond))
    {
        return;
    }

    uint32_t result = 0;
    bool writeResult = true;
    bool updateOverflow = true;

    switch (instruction_.flags.OpCode)
    {
        case 0b0000:  // AND
            result = op1 & op2;
            updateOverflow = false;
            break;
        case 0b0001:  // EOR
            result = op1 ^ op2;
            updateOverflow = false;
            break;
        case 0b0010:  // SUB
            std::tie(carryOut, overflowOut) = Sub32(op1, op2, result);
            break;
        case 0b0011:  // RSB
            std::tie(carryOut, overflowOut) = Sub32(op2, op1, result);
            break;
        case 0b0100:  // ADD
            std::tie(carryOut, overflowOut) = Add32(op1, op2, result);
            break;
        case 0b0101:  // ADC
            std::tie(carryOut, overflowOut) = Add32(op1, op2, result, cpu.registers_.IsCarry());
            break;
        case 0b0110:  // SBC
            std::tie(carryOut, overflowOut) = Sub32(op1, op2, result, true, cpu.registers_.IsCarry());
            break;
        case 0b0111:  // RSC
            std::tie(carryOut, overflowOut) = Sub32(op2, op1, result, true, cpu.registers_.IsCarry());
            break;
        case 0b1000:  // TST
            result = op1 & op2;
            updateOverflow = false;
            writeResult = false;
            break;
        case 0b1001:  // TEQ
            result = op1 ^ op2;
            updateOverflow = false;
            writeResult = false;
            break;
        case 0b1010:  // CMP
            std::tie(carryOut, overflowOut) = Sub32(op1, op2, result);
            writeResult = false;
            break;
        case 0b1011:  // CMN
            std::tie(carryOut, overflowOut) = Add32(op1, op2, result);
            writeResult = false;
            break;
        case 0b1100:  // ORR
            result = op1 | op2;
            updateOverflow = false;
            break;
        case 0b1101:  // MOV
            result = op2;
            updateOverflow = false;
            break;
        case 0b1110:  // BIC
            result = op1 & ~op2;
            updateOverflow = false;
            break;
        case 0b1111:  // MVN
            result = ~op2;
            updateOverflow = false;
            break;
    }

    if (instruction_.flags.S)
    {
        if (destIndex == PC_INDEX)
        {
            cpu.registers_.LoadSPSR();
            InterruptMgr.CheckForInterrupt();
            cpu.flushPipeline_ = writeResult;
        }
        else
        {
            cpu.registers_.SetNegative(result & 0x8000'0000);
            cpu.registers_.SetZero(result == 0);
            cpu.registers_.SetCarry(carryOut);

            if (updateOverflow)
            {
                cpu.registers_.SetOverflow(overflowOut);
            }
        }
    }

    if (writeResult)
    {
        if (!instruction_.flags.S && (destIndex == PC_INDEX))
        {
            cpu.flushPipeline_ = true;
        }

        cpu.registers_.WriteRegister(destIndex, result);
    }
}
}
