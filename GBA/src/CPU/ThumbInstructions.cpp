#include <CPU/ThumbInstructions.hpp>
#include <bit>
#include <cstdint>
#include <utility>
#include <CPU/ARM7TDMI.hpp>
#include <CPU/CpuTypes.hpp>
#include <Logging/Logging.hpp>
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
    return (~(op1 ^ op2) & ((op1 ^ result)) & MSB_32) != 0;
}

/// @brief Determine the result of the overflow flag for subtraction operation: result = op1 - op2
/// @param op1 Subtraction operand
/// @param op2 Subtraction operand
/// @param result Subtraction result
/// @return State of overflow flag after subtraction
bool SubtractionOverflow(uint32_t op1, uint32_t op2, uint32_t result)
{
    return ((op1 ^ op2) & ((op1 ^ result)) & MSB_32) != 0;
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
/// @param carry Carry flag, defaults to 1 meaning non SBC operation.
/// @return Pair of {carry_flag, overflow_flag}.
std::pair<bool, bool> Sub32(uint32_t op1, uint32_t op2, uint32_t& result, bool carry = 1)
{
    uint32_t carryVal = carry ? 0 : 1;
    carryVal = ~carryVal + 1;
    uint64_t result64 = static_cast<uint64_t>(op1) + static_cast<uint64_t>(~op2 + 1) + static_cast<uint64_t>(carryVal);
    result = result64 & MAX_U32;
    bool c = op1 >= op2;
    bool v = SubtractionOverflow(op1, op2, result);
    return {c, v};
}

/// @brief Determine the number of internal cycles needed to perform a multiplication op.
/// @param val Current value in destination register.
/// @return Number of internal cycles.
int InternalMultiplyCycles(uint32_t val)
{
    int cycles;

    if (((val & 0xFFFF'FF00) == 0xFFFF'FF00) || ((val & 0xFFFF'FF00) == 0))
    {
        cycles = 1;
    }
    else if (((val & 0xFFFF'0000) == 0xFFFF'0000) || ((val & 0xFFFF'0000) == 0))
    {
        cycles = 2;
    }
    else if (((val & 0xFF00'0000) == 0xFF00'0000) || ((val & 0xFF00'0000) == 0))
    {
        cycles = 3;
    }
    else
    {
        cycles = 4;
    }

    return cycles;
}
}

namespace CPU::THUMB
{
using namespace Logging;

CPU::Instruction* DecodeInstruction(uint16_t undecodedInstruction, void* buffer)
{
    CPU::Instruction* instructionPtr = nullptr;

    if (SoftwareInterrupt::IsInstanceOf(undecodedInstruction))
    {
        instructionPtr = new(buffer) SoftwareInterrupt(undecodedInstruction);
    }
    else if (UnconditionalBranch::IsInstanceOf(undecodedInstruction))
    {
        instructionPtr = new(buffer) UnconditionalBranch(undecodedInstruction);
    }
    else if (ConditionalBranch::IsInstanceOf(undecodedInstruction))
    {
        instructionPtr = new(buffer) ConditionalBranch(undecodedInstruction);
    }
    else if (MultipleLoadStore::IsInstanceOf(undecodedInstruction))
    {
        instructionPtr = new(buffer) MultipleLoadStore(undecodedInstruction);
    }
    else if (LongBranchWithLink::IsInstanceOf(undecodedInstruction))
    {
        instructionPtr = new(buffer) LongBranchWithLink(undecodedInstruction);
    }
    else if (AddOffsetToStackPointer::IsInstanceOf(undecodedInstruction))
    {
        instructionPtr = new(buffer) AddOffsetToStackPointer(undecodedInstruction);
    }
    else if (PushPopRegisters::IsInstanceOf(undecodedInstruction))
    {
        instructionPtr = new(buffer) PushPopRegisters(undecodedInstruction);
    }
    else if (LoadStoreHalfword::IsInstanceOf(undecodedInstruction))
    {
        instructionPtr = new(buffer) LoadStoreHalfword(undecodedInstruction);
    }
    else if (SPRelativeLoadStore::IsInstanceOf(undecodedInstruction))
    {
        instructionPtr = new(buffer) SPRelativeLoadStore(undecodedInstruction);
    }
    else if (LoadAddress::IsInstanceOf(undecodedInstruction))
    {
        instructionPtr = new(buffer) LoadAddress(undecodedInstruction);
    }
    else if (LoadStoreWithImmediateOffset::IsInstanceOf(undecodedInstruction))
    {
        instructionPtr = new(buffer) LoadStoreWithImmediateOffset(undecodedInstruction);
    }
    else if (LoadStoreWithRegisterOffset::IsInstanceOf(undecodedInstruction))
    {
        instructionPtr = new(buffer) LoadStoreWithRegisterOffset(undecodedInstruction);
    }
    else if (LoadStoreSignExtendedByteHalfword::IsInstanceOf(undecodedInstruction))
    {
        instructionPtr = new(buffer) LoadStoreSignExtendedByteHalfword(undecodedInstruction);
    }
    else if (PCRelativeLoad::IsInstanceOf(undecodedInstruction))
    {
        instructionPtr = new(buffer) PCRelativeLoad(undecodedInstruction);
    }
    else if (HiRegisterOperationsBranchExchange::IsInstanceOf(undecodedInstruction))
    {
        instructionPtr = new(buffer) HiRegisterOperationsBranchExchange(undecodedInstruction);
    }
    else if (ALUOperations::IsInstanceOf(undecodedInstruction))
    {
        instructionPtr = new(buffer) ALUOperations(undecodedInstruction);
    }
    else if (MoveCompareAddSubtractImmediate::IsInstanceOf(undecodedInstruction))
    {
        instructionPtr = new(buffer) MoveCompareAddSubtractImmediate(undecodedInstruction);
    }
    else if (AddSubtract::IsInstanceOf(undecodedInstruction))
    {
        instructionPtr = new(buffer) AddSubtract(undecodedInstruction);
    }
    else if (MoveShiftedRegister::IsInstanceOf(undecodedInstruction))
    {
        instructionPtr = new(buffer) MoveShiftedRegister(undecodedInstruction);
    }

    return instructionPtr;
}

void SoftwareInterrupt::Execute(ARM7TDMI& cpu)
{
    if (LogMgr.LoggingEnabled())
    {
        SetMnemonic(cpu.mnemonic_);
    }

    uint32_t currentCPSR = cpu.registers_.GetCPSR();
    cpu.registers_.SetOperatingState(OperatingState::ARM);
    cpu.registers_.SetOperatingMode(OperatingMode::Supervisor);
    cpu.registers_.WriteRegister(LR_INDEX, cpu.registers_.GetPC() - 2);
    cpu.registers_.SetIrqDisabled(true);
    cpu.registers_.SetSPSR(currentCPSR);
    cpu.registers_.SetPC(SWI_VECTOR);
    cpu.flushPipeline_ = true;
}

void UnconditionalBranch::Execute(ARM7TDMI& cpu)
{
    uint16_t offset = instruction_.Offset11 << 1;
    int16_t signedOffset = SignExtend16(offset, 11);
    uint32_t newPC = cpu.registers_.GetPC() + signedOffset;

    if (LogMgr.LoggingEnabled())
    {
        SetMnemonic(cpu.mnemonic_, newPC);
    }

    cpu.registers_.SetPC(newPC);
    cpu.flushPipeline_ = true;
}

void ConditionalBranch::Execute(ARM7TDMI& cpu)
{
    uint16_t offset = instruction_.SOffset8 << 1;
    int16_t signedOffset = SignExtend16(offset, 8);
    uint32_t newPC = cpu.registers_.GetPC() + signedOffset;

    if (LogMgr.LoggingEnabled())
    {
        SetMnemonic(cpu.mnemonic_, newPC);
    }

    if (cpu.ArmConditionSatisfied(instruction_.Cond))
    {
        cpu.registers_.SetPC(newPC);
        cpu.flushPipeline_ = true;
    }
}

void MultipleLoadStore::Execute(ARM7TDMI& cpu)
{
    if (LogMgr.LoggingEnabled())
    {
        SetMnemonic(cpu.mnemonic_);
    }

    uint8_t regList = instruction_.Rlist;
    uint32_t addr = cpu.registers_.ReadRegister(instruction_.Rb);
    uint32_t wbAddr = addr;
    bool emptyRlist = (regList == 0);
    bool rbInList = (regList & (0x01 << instruction_.Rb));
    bool rbFirstInList = true;

    if (rbInList && !instruction_.L)
    {
        for (uint8_t i = 0x01; i < (0x01 << instruction_.Rb); i <<= 1)
        {
            if (i & regList)
            {
                rbFirstInList = false;
                break;
            }
        }
    }

    if (!rbFirstInList)
    {
        wbAddr += (4 * std::popcount(regList));
    }

    if (instruction_.L)
    {
        // Load
        uint8_t regIndex = 0;

        while (regList != 0)
        {
            if (regList & 0x01)
            {
                auto [value, readCycles] = cpu.ReadMemory(addr, AccessSize::WORD);
                Scheduler.Step(readCycles);
                cpu.registers_.WriteRegister(regIndex, value);
                addr += 4;
            }

            ++regIndex;
            regList >>= 1;
        }

        if (emptyRlist)
        {
            auto [value, readCycles] = cpu.ReadMemory(addr, AccessSize::WORD);
            Scheduler.Step(readCycles);
            cpu.registers_.WriteRegister(PC_INDEX, value);
            cpu.flushPipeline_ = true;
        }
    }
    else
    {
        uint8_t regIndex = 0;

        while (regList != 0)
        {
            if (regList & 0x01)
            {
                uint32_t value = cpu.registers_.ReadRegister(regIndex);

                if (!rbFirstInList && (regIndex == instruction_.Rb))
                {
                    value = wbAddr;
                }

                int writeCycles = cpu.WriteMemory(addr, value, AccessSize::WORD);
                Scheduler.Step(writeCycles);
                addr += 4;
            }

            ++regIndex;
            regList >>= 1;
        }

        if (emptyRlist)
        {
            uint32_t value = cpu.registers_.GetPC() + 2;
            int writeCycles = cpu.WriteMemory(addr, value, AccessSize::WORD);
            Scheduler.Step(writeCycles);
        }
    }

    if (emptyRlist)
    {
        wbAddr = cpu.registers_.ReadRegister(instruction_.Rb) + 0x40;
        cpu.registers_.WriteRegister(instruction_.Rb, wbAddr);
    }
    else if (!(rbInList && instruction_.L))
    {
        cpu.registers_.WriteRegister(instruction_.Rb, addr);
    }

    if (instruction_.L)
    {
        // Run 1 extra internal cycle for final write back of load operation.
        Scheduler.Step(1);
    }
}

void LongBranchWithLink::Execute(ARM7TDMI& cpu)
{
    uint32_t offset = instruction_.Offset;

    if (!instruction_.H)
    {
        // Instruction 1
        offset <<= 12;

        if (offset & 0x0040'0000)
        {
            offset |= 0xFF80'0000;
        }

        uint32_t lr = cpu.registers_.GetPC() + offset;
        cpu.registers_.WriteRegister(LR_INDEX, lr);

        if (LogMgr.LoggingEnabled())
        {
            SetMnemonic(cpu.mnemonic_, 0);
        }
    }
    else
    {
        // Instruction 2
        offset <<= 1;

        uint32_t newPC = cpu.registers_.ReadRegister(LR_INDEX) + offset;
        uint32_t lr = (cpu.registers_.GetPC() - 2) | 0x01;

        if (LogMgr.LoggingEnabled())
        {
            SetMnemonic(cpu.mnemonic_, newPC);
        }

        cpu.registers_.WriteRegister(LR_INDEX, lr);
        cpu.registers_.SetPC(newPC);
        cpu.flushPipeline_ = true;
    }
}

void AddOffsetToStackPointer::Execute(ARM7TDMI& cpu)
{
    uint16_t offset = instruction_.SWord7 << 2;

    if (LogMgr.LoggingEnabled())
    {
        SetMnemonic(cpu.mnemonic_, offset);
    }

    uint32_t newSP = cpu.registers_.GetSP();

    if (instruction_.S)
    {
        newSP -= offset;
    }
    else
    {
        newSP += offset;
    }

    cpu.registers_.WriteRegister(SP_INDEX, newSP);
}

void PushPopRegisters::Execute(ARM7TDMI& cpu)
{
    if (LogMgr.LoggingEnabled())
    {
        SetMnemonic(cpu.mnemonic_);
    }

    // Full Descending stack - Decrement SP and then push, or pop and then increment SP
    uint8_t regList = instruction_.Rlist;
    bool emptyRlist = (regList == 0) && !instruction_.R;
    uint32_t addr = cpu.registers_.GetSP();

    if (instruction_.L)
    {
        // POP
        uint8_t regIndex = 0;

        while (regList != 0)
        {
            if (regList & 0x01)
            {
                auto [value, readCycles] = cpu.ReadMemory(addr, AccessSize::WORD);
                Scheduler.Step(readCycles);
                cpu.registers_.WriteRegister(regIndex, value);
                addr += 4;
            }

            ++regIndex;
            regList >>= 1;
        }

        if (instruction_.R || emptyRlist)
        {
            auto [value, readCycles] = cpu.ReadMemory(addr, AccessSize::WORD);
            Scheduler.Step(readCycles);
            cpu.registers_.WriteRegister(PC_INDEX, value);
            addr += 4;
            cpu.flushPipeline_ = true;
        }
    }
    else
    {
        // PUSH
        if (instruction_.R)
        {
            addr -= 4;
            uint32_t value = cpu.registers_.ReadRegister(LR_INDEX);
            int writeCycles = cpu.WriteMemory(addr, value, AccessSize::WORD);
            Scheduler.Step(writeCycles);
        }
        else if (emptyRlist)
        {
            addr -= 4;
            uint32_t value = cpu.registers_.GetPC() + 2;
            int writeCycles = cpu.WriteMemory(addr, value, AccessSize::WORD);
            Scheduler.Step(writeCycles);
        }

        uint8_t regIndex = 7;

        while (regList != 0)
        {
            if (regList & 0x80)
            {
                addr -= 4;
                uint32_t value = cpu.registers_.ReadRegister(regIndex);
                int writeCycles = cpu.WriteMemory(addr, value, AccessSize::WORD);
                Scheduler.Step(writeCycles);
            }

            --regIndex;
            regList <<= 1;
        }
    }

    if (emptyRlist)
    {
        uint32_t newSP = cpu.registers_.GetSP() + (instruction_.L ? 0x40 : -0x40);
        cpu.registers_.WriteRegister(SP_INDEX, newSP);
    }
    else
    {
        cpu.registers_.WriteRegister(SP_INDEX, addr);
    }

    if (instruction_.L)
    {
        // Run 1 extra internal cycle for final write back of load operation.
        Scheduler.Step(1);
    }
}

void LoadStoreHalfword::Execute(ARM7TDMI& cpu)
{
    if (LogMgr.LoggingEnabled())
    {
        SetMnemonic(cpu.mnemonic_);
    }

    uint32_t addr = cpu.registers_.ReadRegister(instruction_.Rb) + (instruction_.Offset5 << 1);

    if (instruction_.L)
    {
        bool misaligned = addr & 0x01;
        auto [value, readCycles] = cpu.ReadMemory(addr, AccessSize::HALFWORD);
        Scheduler.Step(readCycles);

        if (misaligned)
        {
            value = std::rotr(value, 8);
        }

        cpu.registers_.WriteRegister(instruction_.Rd, value);
    }
    else
    {
        uint16_t value = cpu.registers_.ReadRegister(instruction_.Rd) & MAX_U16;
        int writeCycles = cpu.WriteMemory(addr, value, AccessSize::HALFWORD);
        Scheduler.Step(writeCycles);
    }

    if (instruction_.L)
    {
        // Run 1 extra internal cycle for final write back of load operation.
        Scheduler.Step(1);
    }
}

void SPRelativeLoadStore::Execute(ARM7TDMI& cpu)
{
    if (LogMgr.LoggingEnabled())
    {
        SetMnemonic(cpu.mnemonic_);
    }

    uint32_t addr = cpu.registers_.GetSP() + (instruction_.Word8 << 2);

    if (instruction_.L)
    {
        auto [value, readCycles] = cpu.ReadMemory(addr, AccessSize::WORD);
        Scheduler.Step(readCycles);

        if (addr & 0x03)
        {
            value = std::rotr(value, ((addr & 0x03) * 8));
        }

        cpu.registers_.WriteRegister(instruction_.Rd, value);
    }
    else
    {
        uint32_t value = cpu.registers_.ReadRegister(instruction_.Rd);
        int writeCycles = cpu.WriteMemory(addr, value, AccessSize::WORD);
        Scheduler.Step(writeCycles);
    }

    if (instruction_.L)
    {
        // Run 1 extra internal cycle for final write back of load operation.
        Scheduler.Step(1);
    }
}

void LoadAddress::Execute(ARM7TDMI& cpu)
{
    uint8_t destIndex = instruction_.Rd;
    uint16_t offset = (instruction_.Word8 << 2);

    if (LogMgr.LoggingEnabled())
    {
        SetMnemonic(cpu.mnemonic_, destIndex, offset);
    }

    uint32_t addr = 0;

    if (instruction_.SP)
    {
        addr = cpu.registers_.GetSP();
    }
    else
    {
        addr = (cpu.registers_.GetPC() & 0xFFFF'FFFD);
    }

    addr += offset;
    cpu.registers_.WriteRegister(destIndex, addr);
}

void LoadStoreWithImmediateOffset::Execute(ARM7TDMI& cpu)
{
    if (LogMgr.LoggingEnabled())
    {
        SetMnemonic(cpu.mnemonic_);
    }

    AccessSize alignment = instruction_.B ? AccessSize::BYTE : AccessSize::WORD;
    uint8_t offset = instruction_.B ? instruction_.Offset5 : (instruction_.Offset5 << 2);
    uint32_t addr = cpu.registers_.ReadRegister(instruction_.Rb) + offset;

    if (instruction_.L)
    {
        // Load
        auto [value, readCycles] = cpu.ReadMemory(addr, alignment);
        Scheduler.Step(readCycles);

        if ((alignment == AccessSize::WORD) && (addr & 0x03))
        {
            value = std::rotr(value, ((addr & 0x03) * 8));
        }

        cpu.registers_.WriteRegister(instruction_.Rd, value);
    }
    else
    {
        // Store
        uint32_t value = cpu.registers_.ReadRegister(instruction_.Rd);
        int writeCycles = cpu.WriteMemory(addr, value, alignment);
        Scheduler.Step(writeCycles);
    }

    if (instruction_.L)
    {
        // Run 1 extra internal cycle for final write back of load operation.
        Scheduler.Step(1);
    }
}

void LoadStoreWithRegisterOffset::Execute(ARM7TDMI& cpu)
{
    if (LogMgr.LoggingEnabled())
    {
        SetMnemonic(cpu.mnemonic_);
    }

    uint32_t addr = cpu.registers_.ReadRegister(instruction_.Rb) + cpu.registers_.ReadRegister(instruction_.Ro);
    AccessSize alignment = (instruction_.B) ? AccessSize::BYTE : AccessSize::WORD;

    if (instruction_.L)
    {
        // Load
        auto [value, readCycles] = cpu.ReadMemory(addr, alignment);
        Scheduler.Step(readCycles);

        if ((alignment == AccessSize::WORD) && (addr & 0x03))
        {
            value = std::rotr(value, ((addr & 0x03) * 8));
        }

        cpu.registers_.WriteRegister(instruction_.Rd, value);
    }
    else
    {
        // Store
        uint32_t value = cpu.registers_.ReadRegister(instruction_.Rd);
        int writeCycles = cpu.WriteMemory(addr, value, alignment);
        Scheduler.Step(writeCycles);
    }

    if (instruction_.L)
    {
        // Run 1 extra internal cycle for final write back of load operation.
        Scheduler.Step(1);
    }
}

void LoadStoreSignExtendedByteHalfword::Execute(ARM7TDMI& cpu)
{
    if (LogMgr.LoggingEnabled())
    {
        SetMnemonic(cpu.mnemonic_);
    }

    uint32_t addr = cpu.registers_.ReadRegister(instruction_.Rb) + cpu.registers_.ReadRegister(instruction_.Ro);
    bool isLoad = false;

    if (instruction_.S)
    {
        uint32_t value;
        int readCycles;
        isLoad = true;
        bool h = instruction_.H;

        if (h && (addr & 0x01))
        {
            // Convert LDRSH into LDRSB
            h = false;
        }

        if (h)
        {
            // LDSH
            std::tie(value, readCycles) = cpu.ReadMemory(addr, AccessSize::HALFWORD);
            value = SignExtend32(value, 15);
        }
        else
        {
            // LDSB
            std::tie(value, readCycles) = cpu.ReadMemory(addr, AccessSize::BYTE);
            value = SignExtend32(value, 7);
        }

        Scheduler.Step(readCycles);
        cpu.registers_.WriteRegister(instruction_.Rd, value);
    }
    else
    {
        if (instruction_.H)
        {
            // LDRH
            isLoad = true;
            auto [value, readCycles] = cpu.ReadMemory(addr, AccessSize::HALFWORD);
            Scheduler.Step(readCycles);

            if (addr & 0x01)
            {
                value = std::rotr(value, 8);
            }

            cpu.registers_.WriteRegister(instruction_.Rd, value);
        }
        else
        {
            // STRH
            uint32_t value = cpu.registers_.ReadRegister(instruction_.Rd);
            int writeCycles = cpu.WriteMemory(addr, value, AccessSize::HALFWORD);
            Scheduler.Step(writeCycles);
        }
    }

    if (isLoad)
    {
        Scheduler.Step(1);
        cpu.flushPipeline_ = (instruction_.Rd == PC_INDEX);
    }
}

void PCRelativeLoad::Execute(ARM7TDMI& cpu)
{
    if (LogMgr.LoggingEnabled())
    {
        SetMnemonic(cpu.mnemonic_);
    }

    uint32_t addr = (cpu.registers_.GetPC() & 0xFFFF'FFFC) + (instruction_.Word8 << 2);
    auto [value, readCycles] = cpu.ReadMemory(addr, AccessSize::WORD);
    Scheduler.Step(readCycles);

    if (addr & 0x03)
    {
        value = std::rotr(value, ((addr & 0x03) * 8));
    }

    cpu.registers_.WriteRegister(instruction_.Rd, value);
}

void HiRegisterOperationsBranchExchange::Execute(ARM7TDMI& cpu)
{
    uint8_t destIndex = instruction_.RdHd;
    uint8_t srcIndex = instruction_.RsHs;

    if (instruction_.H1)
    {
        destIndex += 8;
    }

    if (instruction_.H2)
    {
        srcIndex += 8;
    }

    if (LogMgr.LoggingEnabled())
    {
        SetMnemonic(cpu.mnemonic_, destIndex, srcIndex);
    }

    switch (instruction_.Op)
    {
        case 0b00:  // ADD
        {
            uint32_t result = cpu.registers_.ReadRegister(destIndex) + cpu.registers_.ReadRegister(srcIndex);
            cpu.registers_.WriteRegister(destIndex, result);
            cpu.flushPipeline_ = (destIndex == PC_INDEX);
            break;
        }
        case 0b01:  // CMP
        {
            uint32_t op1 = cpu.registers_.ReadRegister(destIndex);
            uint32_t op2 = cpu.registers_.ReadRegister(srcIndex);
            uint32_t result = 0;

            auto [c, v] = Sub32(op1, op2, result);

            cpu.registers_.SetNegative(result & MSB_32);
            cpu.registers_.SetZero(result == 0);
            cpu.registers_.SetCarry(c);
            cpu.registers_.SetOverflow(v);
            break;
        }
        case 0b10:  // MOV
            cpu.registers_.WriteRegister(destIndex, cpu.registers_.ReadRegister(srcIndex));
            cpu.flushPipeline_ = (destIndex == PC_INDEX);
            break;
        case 0b11:  // BX
        {
            uint32_t newPC = cpu.registers_.ReadRegister(srcIndex);
            cpu.flushPipeline_ = true;

            if (newPC & 0x01)
            {
                cpu.registers_.SetOperatingState(OperatingState::THUMB);
            }
            else
            {
                cpu.registers_.SetOperatingState(OperatingState::ARM);
            }

            cpu.registers_.WriteRegister(PC_INDEX, newPC);
            break;
        }
    }
}

void ALUOperations::Execute(ARM7TDMI& cpu)
{
    if (LogMgr.LoggingEnabled())
    {
        SetMnemonic(cpu.mnemonic_);
    }

    bool storeResult = true;
    bool updateCarry = true;
    bool updateOverflow = true;
    bool carryOut = cpu.registers_.IsCarry();
    bool overflowOut = cpu.registers_.IsOverflow();

    uint32_t result = 0;
    uint32_t op1 = cpu.registers_.ReadRegister(instruction_.Rd);
    uint32_t op2 = cpu.registers_.ReadRegister(instruction_.Rs);

    switch (instruction_.Op)
    {
        case 0b0000:  // AND
            result = op1 & op2;
            updateCarry = false;
            updateOverflow = false;
            break;
        case 0b0001:  // EOR
            result = op1 ^ op2;
            updateCarry = false;
            updateOverflow = false;
            break;
        case 0b0010:  // LSL
        {
            op2 &= 0xFF;
            result = op1;
            updateOverflow = false;

            if (op2 > 32)
            {
                carryOut = false;
                result = 0;
            }
            else if (op2 == 32)
            {
                carryOut = (op1 & 0x01);
                result = 0;
            }
            else if (op2 != 0)
            {
                carryOut = (op1 & (MSB_32 >> (op2 - 1)));
                result <<= op2;
            }

            Scheduler.Step(1);
            break;
        }
        case 0b0011:  // LSR
        {
            op2 &= 0xFF;
            result = op1;
            updateOverflow = false;

            if (op2 > 32)
            {
                carryOut = false;
                result = 0;
            }
            else if (op2 == 32)
            {
                carryOut = (op1 & MSB_32);
                result = 0;
            }
            else if (op2 != 0)
            {
                carryOut = (op1 & (0x01 << (op2 - 1)));
                result >>= op2;
            }

            Scheduler.Step(1);
            break;
        }
        case 0b0100:  // ASR
        {
            op2 &= 0xFF;
            result = op1;
            updateOverflow = false;

            bool msbSet = op1 & MSB_32;

            if (op2 >= 32)
            {
                carryOut = msbSet;
                result = msbSet ? 0xFFFF'FFFF : 0;
            }
            else if (op2 > 0)
            {
                carryOut = (op1 & (0x01 << (op2 - 1)));

                while (op2 > 0)
                {
                    result >>= 1;
                    result |= (msbSet ? MSB_32 : 0);
                    --op2;
                }
            }

            Scheduler.Step(1);
            break;
        }
        case 0b0101:  // ADC
            std::tie(carryOut, overflowOut) = Add32(op1, op2, result, carryOut);
            break;
        case 0b0110:  // SBC
            std::tie(carryOut, overflowOut) = Sub32(op1, op2, result, carryOut);
            break;
        case 0b0111:  // ROR
        {
            op2 &= 0xFF;
            result = op1;
            updateOverflow = false;

            if (op2 > 32)
            {
                op2 %= 32;
            }

            if (op2)
            {
                carryOut = (op1 & (0x01 << (op2 - 1)));
                result = std::rotr(result, op2);
            }

            Scheduler.Step(1);
            break;
        }
        case 0b1000:  // TST
            result = op1 & op2;
            storeResult = false;
            updateCarry = false;
            updateOverflow = false;
            break;
        case 0b1001:  // NEG
            std::tie(carryOut, overflowOut) = Sub32(0, op2, result);
            break;
        case 0b1010:  // CMP
            std::tie(carryOut, overflowOut) = Sub32(op1, op2, result);
            storeResult = false;
            break;
        case 0b1011:  // CMN
            std::tie(carryOut, overflowOut) = Add32(op1, op2, result);
            storeResult = false;
            break;
        case 0b1100:  // ORR
            result = op1 | op2;
            updateCarry = false;
            updateOverflow = false;
            break;
        case 0b1101:  // MUL
            result = op1 * op2;
            updateOverflow = false;
            Scheduler.Step(InternalMultiplyCycles(op1));
            break;
        case 0b1110:  // BIC
            result = op1 & ~op2;
            updateCarry = false;
            updateOverflow = false;
            break;
        case 0b1111:  // MVN
            result = ~op2;
            updateCarry = false;
            updateOverflow = false;
            break;
    }

    cpu.registers_.SetNegative(result & MSB_32);
    cpu.registers_.SetZero(result == 0);

    if (updateCarry)
    {
        cpu.registers_.SetCarry(carryOut);
    }

    if (updateOverflow)
    {
        cpu.registers_.SetOverflow(overflowOut);
    }

    if (storeResult)
    {
        cpu.registers_.WriteRegister(instruction_.Rd, result);
    }
}

void MoveCompareAddSubtractImmediate::Execute(ARM7TDMI& cpu)
{
    if (LogMgr.LoggingEnabled())
    {
        SetMnemonic(cpu.mnemonic_);
    }

    bool carryOut = cpu.registers_.IsCarry();
    bool overflowOut = cpu.registers_.IsOverflow();
    bool saveResult = true;
    bool updateAllFlags = true;
    uint32_t op1 = cpu.registers_.ReadRegister(instruction_.Rd);
    uint32_t op2 = instruction_.Offset8;
    uint32_t result = 0;

    switch (instruction_.Op)
    {
        case 0b00:  // MOV
            result = op2;
            updateAllFlags = false;
            break;
        case 0b01:  // CMP
            std::tie(carryOut, overflowOut) = Sub32(op1, op2, result);
            saveResult = false;
            break;
        case 0b10:  // ADD
            std::tie(carryOut, overflowOut) = Add32(op1, op2, result);
            break;
        case 0b11:  // SUB
            std::tie(carryOut, overflowOut) = Sub32(op1, op2, result);
            break;
    }

    cpu.registers_.SetNegative(result & MSB_32);
    cpu.registers_.SetZero(result == 0);

    if (updateAllFlags)
    {
        cpu.registers_.SetCarry(carryOut);
        cpu.registers_.SetOverflow(overflowOut);
    }

    if (saveResult)
    {
        cpu.registers_.WriteRegister(instruction_.Rd, result);
    }
}

void AddSubtract::Execute(ARM7TDMI& cpu)
{
    if (LogMgr.LoggingEnabled())
    {
        SetMnemonic(cpu.mnemonic_);
    }

    uint32_t op1 = cpu.registers_.ReadRegister(instruction_.Rs);
    uint32_t op2 = instruction_.I ? instruction_.RnOffset3 : cpu.registers_.ReadRegister(instruction_.RnOffset3);
    bool carryOut;
    bool overflowOut;
    uint32_t result;

    if (instruction_.Op)
    {
        std::tie(carryOut, overflowOut) = Sub32(op1, op2, result);
    }
    else
    {
        std::tie(carryOut, overflowOut) = Add32(op1, op2, result);
    }

    cpu.registers_.SetNegative(result & MSB_32);
    cpu.registers_.SetZero(result == 0);
    cpu.registers_.SetCarry(carryOut);
    cpu.registers_.SetOverflow(overflowOut);
    cpu.registers_.WriteRegister(instruction_.Rd, result);
}

void MoveShiftedRegister::Execute(ARM7TDMI& cpu)
{
    if (LogMgr.LoggingEnabled())
    {
        SetMnemonic(cpu.mnemonic_);
    }

    bool carryOut = cpu.registers_.IsCarry();

    uint32_t result = 0;
    uint32_t operand = cpu.registers_.ReadRegister(instruction_.Rs);
    uint8_t shiftAmount = instruction_.Offset5;

    switch (instruction_.Op)
    {
        case 0b00:  // LSL
        {
            if (shiftAmount == 0)
            {
                result = operand;
            }
            else
            {
                carryOut = (operand & (MSB_32 >> (shiftAmount - 1)));
                result = operand << shiftAmount;
            }

            break;
        }
        case 0b01:  // LSR
        {
            if (shiftAmount == 0)
            {
                // LSR #0 -> LSR #32
                carryOut = (operand & MSB_32);
                result = 0;
            }
            else
            {
                carryOut = (operand & (0x01 << (shiftAmount - 1)));
                result = operand >> shiftAmount;
            }

            break;
        }
        case 0b10:  // ASR
        {
            bool msbSet = operand & MSB_32;

            if (shiftAmount == 0)
            {
                // ASR #0 -> ASR #32
                carryOut = msbSet;
                result = msbSet ? 0xFFFF'FFFF : 0;
            }
            else
            {
                carryOut = (operand & (0x01 << (shiftAmount - 1)));

                for (int i = 0; i < shiftAmount; ++i)
                {
                    operand >>= 1;
                    operand |= (msbSet ? MSB_32 : 0);
                }

                result = operand;
            }

            break;
        }
    }

    cpu.registers_.SetNegative(result & MSB_32);
    cpu.registers_.SetZero(result == 0);
    cpu.registers_.SetCarry(carryOut);
    cpu.registers_.WriteRegister(instruction_.Rd, result);
}
}
