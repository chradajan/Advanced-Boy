#include <ARM7TDMI/ThumbInstructions.hpp>
#include <ARM7TDMI/ARM7TDMI.hpp>
#include <Config.hpp>
#include <System/MemoryMap.hpp>
#include <bit>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <utility>

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
}
namespace CPU::THUMB
{
std::unique_ptr<ThumbInstruction> DecodeInstruction(uint16_t const instruction)
{
    if (SoftwareInterrupt::IsInstanceOf(instruction))
    {
        return std::make_unique<SoftwareInterrupt>(instruction);
    }
    else if (UnconditionalBranch::IsInstanceOf(instruction))
    {
        return std::make_unique<UnconditionalBranch>(instruction);
    }
    else if (ConditionalBranch::IsInstanceOf(instruction))
    {
        return std::make_unique<ConditionalBranch>(instruction);
    }
    else if (MultipleLoadStore::IsInstanceOf(instruction))
    {
        return std::make_unique<MultipleLoadStore>(instruction);
    }
    else if (LongBranchWithLink::IsInstanceOf(instruction))
    {
        return std::make_unique<LongBranchWithLink>(instruction);
    }
    else if (AddOffsetToStackPointer::IsInstanceOf(instruction))
    {
        return std::make_unique<AddOffsetToStackPointer>(instruction);
    }
    else if (PushPopRegisters::IsInstanceOf(instruction))
    {
        return std::make_unique<PushPopRegisters>(instruction);
    }
    else if (LoadStoreHalfword::IsInstanceOf(instruction))
    {
        return std::make_unique<LoadStoreHalfword>(instruction);
    }
    else if (SPRelativeLoadStore::IsInstanceOf(instruction))
    {
        return std::make_unique<SPRelativeLoadStore>(instruction);
    }
    else if (LoadAddress::IsInstanceOf(instruction))
    {
        return std::make_unique<LoadAddress>(instruction);
    }
    else if (LoadStoreWithImmediateOffset::IsInstanceOf(instruction))
    {
        return std::make_unique<LoadStoreWithImmediateOffset>(instruction);
    }
    else if (LoadStoreWithRegisterOffset::IsInstanceOf(instruction))
    {
        return std::make_unique<LoadStoreWithRegisterOffset>(instruction);
    }
    else if (LoadStoreSignExtendedByteHalfword::IsInstanceOf(instruction))
    {
        return std::make_unique<LoadStoreSignExtendedByteHalfword>(instruction);
    }
    else if (PCRelativeLoad::IsInstanceOf(instruction))
    {
        return std::make_unique<PCRelativeLoad>(instruction);
    }
    else if (HiRegisterOperationsBranchExchange::IsInstanceOf(instruction))
    {
        return std::make_unique<HiRegisterOperationsBranchExchange>(instruction);
    }
    else if (ALUOperations::IsInstanceOf(instruction))
    {
        return std::make_unique<ALUOperations>(instruction);
    }
    else if (MoveCompareAddSubtractImmediate::IsInstanceOf(instruction))
    {
        return std::make_unique<MoveCompareAddSubtractImmediate>(instruction);
    }
    else if (AddSubtract::IsInstanceOf(instruction))
    {
        return std::make_unique<AddSubtract>(instruction);
    }
    else if (MoveShiftedRegister::IsInstanceOf(instruction))
    {
        return std::make_unique<MoveShiftedRegister>(instruction);
    }

    return nullptr;
}

int SoftwareInterrupt::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: THUMB_SoftwareInterrupt");
}

int UnconditionalBranch::Execute(ARM7TDMI& cpu)
{
    int16_t signedOffset = instruction_.flags.Offset11 << 1;

    if (signedOffset & 0x0800)
    {
        signedOffset |= 0xF000;
    }

    uint32_t newPC = cpu.registers_.GetPC() + signedOffset;

    if (Config::LOGGING_ENABLED)
    {
        SetMnemonic(newPC);
    }

    cpu.registers_.SetPC(newPC);
    cpu.flushPipeline_ = true;

    return 1;
}

int ConditionalBranch::Execute(ARM7TDMI& cpu)
{
    int16_t signedOffset = instruction_.flags.SOffset8 << 1;

    if (signedOffset & 0x0100)
    {
        signedOffset |= 0xFE00;
    }

    uint32_t newPC = cpu.registers_.GetPC() + signedOffset;

    if (Config::LOGGING_ENABLED)
    {
        SetMnemonic(newPC);
    }

    if (cpu.ArmConditionMet(instruction_.flags.Cond))
    {
        cpu.registers_.SetPC(newPC);
        cpu.flushPipeline_ = true;
    }

    return 1;
}

int MultipleLoadStore::Execute(ARM7TDMI& cpu)
{
    int cycles = 1;

    if (Config::LOGGING_ENABLED)
    {
        SetMnemonic();
    }

    uint8_t regList = instruction_.flags.Rlist;
    uint32_t addr = cpu.registers_.ReadRegister(instruction_.flags.Rb);
    uint32_t wbAddr = addr;
    bool emptyRlist = (regList == 0);
    bool rbInList = (regList & (0x01 << instruction_.flags.Rb));
    bool rbFirstInList = true;

    if (rbInList && !instruction_.flags.L)
    {
        for (uint8_t i = 0x01; i < (0x01 << instruction_.flags.Rb); i <<= 1)
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

    if (instruction_.flags.L)
    {
        // Load
        ++cycles;
        uint8_t regIndex = 0;

        while (regList != 0)
        {
            if (regList & 0x01)
            {
                auto [value, readCycles] = cpu.ReadMemory(addr, AccessSize::WORD);
                cycles += readCycles;
                cpu.registers_.WriteRegister(regIndex, value);
                addr += 4;
            }

            ++regIndex;
            regList >>= 1;
        }

        if (emptyRlist)
        {
            auto [value, readCycles] = cpu.ReadMemory(addr, AccessSize::WORD);
            cycles += readCycles;
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

                if (!rbFirstInList && (regIndex == instruction_.flags.Rb))
                {
                    value = wbAddr;
                }

                int writeCycles = cpu.WriteMemory(addr, value, AccessSize::WORD);
                cycles += writeCycles;
                addr += 4;
            }

            ++regIndex;
            regList >>= 1;
        }

        if (emptyRlist)
        {
            uint32_t value = cpu.registers_.GetPC() + 2;
            int writeCycles = cpu.WriteMemory(addr, value, AccessSize::WORD);
            cycles += writeCycles;
        }
    }

    if (emptyRlist)
    {
        wbAddr = cpu.registers_.ReadRegister(instruction_.flags.Rb) + 0x40;
        cpu.registers_.WriteRegister(instruction_.flags.Rb, wbAddr);
    }
    else if (!(rbInList && instruction_.flags.L))
    {
        cpu.registers_.WriteRegister(instruction_.flags.Rb, addr);
    }

    return cycles;
}

int LongBranchWithLink::Execute(ARM7TDMI& cpu)
{
    uint32_t offset = instruction_.flags.Offset;

    if (!instruction_.flags.H)
    {
        // Instruction 1
        offset <<= 12;

        if (offset & 0x0040'0000)
        {
            offset |= 0xFF80'0000;
        }

        uint32_t lr = cpu.registers_.GetPC() + offset;
        cpu.registers_.WriteRegister(LR_INDEX, lr);

        if (Config::LOGGING_ENABLED)
        {
            SetMnemonic(0);
        }
    }
    else
    {
        // Instruction 2
        offset <<= 1;

        uint32_t newPC = cpu.registers_.ReadRegister(LR_INDEX) + offset;
        uint32_t lr = (cpu.registers_.GetPC() - 2) | 0x01;

        if (Config::LOGGING_ENABLED)
        {
            SetMnemonic(newPC);
        }

        cpu.registers_.WriteRegister(LR_INDEX, lr);
        cpu.registers_.SetPC(newPC);
        cpu.flushPipeline_ = true;
    }

    return 1;
}

int AddOffsetToStackPointer::Execute(ARM7TDMI& cpu)
{
    uint16_t offset = instruction_.flags.SWord7;
    offset <<= 2;
    int16_t signedOffset = instruction_.flags.S ? -offset : offset;

    if (Config::LOGGING_ENABLED)
    {
        SetMnemonic(offset);
    }

    uint32_t sp = cpu.registers_.GetSP();
    cpu.registers_.WriteRegister(SP_INDEX, sp + signedOffset);
    return 1;
}

int PushPopRegisters::Execute(ARM7TDMI& cpu)
{
    int cycles = 1;

    if (Config::LOGGING_ENABLED)
    {
        SetMnemonic();
    }

    // Full Descending stack - Decrement SP and then push, or pop and then increment SP
    uint8_t regList = instruction_.flags.Rlist;
    bool emptyRlist = (regList == 0) && !instruction_.flags.R;
    uint32_t addr = cpu.registers_.GetSP();

    if (instruction_.flags.L)
    {
        // POP
        ++cycles;
        uint8_t regIndex = 0;

        while (regList != 0)
        {
            if (regList & 0x01)
            {
                auto [value, readCycles] = cpu.ReadMemory(addr, AccessSize::WORD);
                cycles += readCycles;
                cpu.registers_.WriteRegister(regIndex, value);
                addr += 4;
            }

            ++regIndex;
            regList >>= 1;
        }

        if (instruction_.flags.R || emptyRlist)
        {
            auto [value, readCycles] = cpu.ReadMemory(addr, AccessSize::WORD);
            cycles += readCycles;
            cpu.registers_.WriteRegister(PC_INDEX, value);
            addr += 4;
            cpu.flushPipeline_ = true;
        }
    }
    else
    {
        // PUSH
        if (instruction_.flags.R)
        {
            addr -= 4;
            uint32_t value = cpu.registers_.ReadRegister(LR_INDEX);
            int writeCycles = cpu.WriteMemory(addr, value, AccessSize::WORD);
            cycles += writeCycles;
        }
        else if (emptyRlist)
        {
            addr -= 4;
            uint32_t value = cpu.registers_.GetPC() + 2;
            int writeCycles = cpu.WriteMemory(addr, value, AccessSize::WORD);
            cycles += writeCycles;
        }

        uint8_t regIndex = 7;

        while (regList != 0)
        {
            if (regList & 0x80)
            {
                addr -= 4;
                uint32_t value = cpu.registers_.ReadRegister(regIndex);
                int writeCycles = cpu.WriteMemory(addr, value, AccessSize::WORD);
                cycles += writeCycles;
            }

            --regIndex;
            regList <<= 1;
        }
    }

    if (emptyRlist)
    {
        uint32_t newSP = cpu.registers_.GetSP() + (instruction_.flags.L ? 0x40 : -0x40);
        cpu.registers_.WriteRegister(SP_INDEX, newSP);
    }
    else
    {
        cpu.registers_.WriteRegister(SP_INDEX, addr);
    }

    return cycles;
}

int LoadStoreHalfword::Execute(ARM7TDMI& cpu)
{
    int cycles = 1;

    if (Config::LOGGING_ENABLED)
    {
        SetMnemonic();
    }

    uint32_t addr = cpu.registers_.ReadRegister(instruction_.flags.Rb) + (instruction_.flags.Offset5 << 1);

    if (instruction_.flags.L)
    {
        ++cycles;
        bool misaligned = addr & 0x01;
        auto [value, readCycles] = cpu.ReadMemory(addr, AccessSize::HALFWORD);

        if (misaligned)
        {
            value = std::rotr(value, 8);
        }

        cpu.registers_.WriteRegister(instruction_.flags.Rd, value);
        cycles += readCycles;
    }
    else
    {
        uint16_t value = cpu.registers_.ReadRegister(instruction_.flags.Rd) & MAX_U16;
        int writeCycles = cpu.WriteMemory(addr, value, AccessSize::HALFWORD);
        cycles += writeCycles;
    }

    return cycles;
}

int SPRelativeLoadStore::Execute(ARM7TDMI& cpu)
{
    int cycles = 1;

    if (Config::LOGGING_ENABLED)
    {
        SetMnemonic();
    }

    uint32_t addr = cpu.registers_.GetSP() + (instruction_.flags.Word8 << 2);

    if (instruction_.flags.L)
    {
        ++cycles;
        auto [value, readCycles] = cpu.ReadMemory(addr, AccessSize::WORD);
        cycles += readCycles;

        if (addr & 0x03)
        {
            value = std::rotr(value, ((addr & 0x03) * 8));
        }

        cpu.registers_.WriteRegister(instruction_.flags.Rd, value);
    }
    else
    {
        uint32_t value = cpu.registers_.ReadRegister(instruction_.flags.Rd);
        int writeCycles = cpu.WriteMemory(addr, value, AccessSize::WORD);
        cycles += writeCycles;
    }

    return cycles;
}

int LoadAddress::Execute(ARM7TDMI& cpu)
{
    uint8_t destIndex = instruction_.flags.Rd;
    uint16_t offset = (instruction_.flags.Word8 << 2);

    if (Config::LOGGING_ENABLED)
    {
        SetMnemonic(destIndex, offset);
    }

    uint32_t addr;

    if (instruction_.flags.SP)
    {
        addr = cpu.registers_.GetSP();
    }
    else
    {
        addr = (cpu.registers_.GetPC() & 0xFFFF'FFFD);
    }

    addr += offset;
    cpu.registers_.WriteRegister(destIndex, addr);
    return 1;
}

int LoadStoreWithImmediateOffset::Execute(ARM7TDMI& cpu)
{
    int cycles = 1;

    if (Config::LOGGING_ENABLED)
    {
        SetMnemonic();
    }

    AccessSize alignment = (instruction_.flags.B) ? AccessSize::BYTE : AccessSize::WORD;
    uint8_t offset = (instruction_.flags.B) ? instruction_.flags.Offset5 : (instruction_.flags.Offset5 << 2);
    uint32_t addr = cpu.registers_.ReadRegister(instruction_.flags.Rb) + offset;

    if (instruction_.flags.L)
    {
        // Load
        ++cycles;
        auto [value, readCycles] = cpu.ReadMemory(addr, alignment);
        cycles += readCycles;

        if ((alignment == AccessSize::WORD) && (addr & 0x03))
        {
            value = std::rotr(value, ((addr & 0x03) * 8));
        }

        cpu.registers_.WriteRegister(instruction_.flags.Rd, value);
    }
    else
    {
        // Store
        uint32_t value = cpu.registers_.ReadRegister(instruction_.flags.Rd);
        int writeCycles = cpu.WriteMemory(addr, value, alignment);
        cycles += writeCycles;
    }

    return cycles;
}

int LoadStoreWithRegisterOffset::Execute(ARM7TDMI& cpu)
{
    int cycles = 1;

    if (Config::LOGGING_ENABLED)
    {
        SetMnemonic();
    }

    uint32_t addr = cpu.registers_.ReadRegister(instruction_.flags.Rb) + cpu.registers_.ReadRegister(instruction_.flags.Ro);
    AccessSize alignment = (instruction_.flags.B) ? AccessSize::BYTE : AccessSize::WORD;

    if (instruction_.flags.L)
    {
        // Load
        ++cycles;
        auto [value, readCycles] = cpu.ReadMemory(addr, alignment);
        cycles += readCycles;

        if ((alignment == AccessSize::WORD) && (addr & 0x03))
        {
            value = std::rotr(value, ((addr & 0x03) * 8));
        }

        cpu.registers_.WriteRegister(instruction_.flags.Rd, value);
    }
    else
    {
        // Store
        uint32_t value = cpu.registers_.ReadRegister(instruction_.flags.Rd);
        int writeCycles = cpu.WriteMemory(addr, value, alignment);
        cycles += writeCycles;
    }

    return cycles;
}

int LoadStoreSignExtendedByteHalfword::Execute(ARM7TDMI& cpu)
{
    int cycles = 1;

    if (Config::LOGGING_ENABLED)
    {
        SetMnemonic();
    }

    uint32_t addr = cpu.registers_.ReadRegister(instruction_.flags.Rb) + cpu.registers_.ReadRegister(instruction_.flags.Ro);
    bool isLoad = false;

    if (instruction_.flags.S)
    {
        uint32_t value;
        int readCycles;
        isLoad = true;
        bool h = instruction_.flags.H;

        if (h && (addr & 0x01))
        {
            // Convert LDRSH into LDRSB
            h = false;
        }

        if (h)
        {
            // LDSH
            std::tie(value, readCycles) = cpu.ReadMemory(addr, AccessSize::HALFWORD);

            if (value & 0x8000)
            {
                value |= 0xFFFF'0000;
            }
        }
        else
        {
            // LDSB
            std::tie(value, readCycles) = cpu.ReadMemory(addr, AccessSize::BYTE);

            if (value & 0x80)
            {
                value |= 0xFFFF'FF00;
            }
        }

        cycles += readCycles;
        cpu.registers_.WriteRegister(instruction_.flags.Rd, value);
    }
    else
    {
        if (instruction_.flags.H)
        {
            // LDRH
            isLoad = true;
            auto [value, readCycles] = cpu.ReadMemory(addr, AccessSize::HALFWORD);
            cycles += readCycles;

            if (addr & 0x01)
            {
                value = std::rotr(value, 8);
            }

            cpu.registers_.WriteRegister(instruction_.flags.Rd, value);
        }
        else
        {
            // STRH
            uint32_t value = cpu.registers_.ReadRegister(instruction_.flags.Rd);
            int writeCycles = cpu.WriteMemory(addr, value, AccessSize::HALFWORD);
            cycles += writeCycles;
        }
    }

    if (isLoad)
    {
        ++cycles;
        cpu.flushPipeline_ = (instruction_.flags.Rd == PC_INDEX);
    }


    return cycles;
}

int PCRelativeLoad::Execute(ARM7TDMI& cpu)
{
    int cycles = 1;

    if (Config::LOGGING_ENABLED)
    {
        SetMnemonic();
    }

    uint32_t addr = (cpu.registers_.GetPC() & 0xFFFF'FFFC) + (instruction_.flags.Word8 << 2);
    auto [value, readCycles] = cpu.ReadMemory(addr, AccessSize::WORD);

    if (addr & 0x03)
    {
        value = std::rotr(value, ((addr & 0x03) * 8));
    }

    cycles += readCycles;
    cpu.registers_.WriteRegister(instruction_.flags.Rd, value);
    return cycles;
}

int HiRegisterOperationsBranchExchange::Execute(ARM7TDMI& cpu)
{
    uint8_t destIndex = instruction_.flags.RdHd;
    uint8_t srcIndex = instruction_.flags.RsHs;

    if (instruction_.flags.H1)
    {
        destIndex += 8;
    }

    if (instruction_.flags.H2)
    {
        srcIndex += 8;
    }

    if (Config::LOGGING_ENABLED)
    {
        SetMnemonic(destIndex, srcIndex);
    }

    switch (instruction_.flags.Op)
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
            uint32_t result;

            auto [c, v] = Sub32(op1, op2, result);

            cpu.registers_.SetNegative(result & 0x8000'0000);
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

    return 1;
}

int ALUOperations::Execute(ARM7TDMI& cpu)
{
    int cycles = 1;

    if (Config::LOGGING_ENABLED)
    {
        SetMnemonic();
    }

    bool storeResult = true;
    bool updateCarry = true;
    bool updateOverflow = true;
    bool carryOut = cpu.registers_.IsCarry();
    bool overflowOut = cpu.registers_.IsOverflow();

    uint32_t result;
    uint32_t op1 = cpu.registers_.ReadRegister(instruction_.flags.Rd);
    uint32_t op2 = cpu.registers_.ReadRegister(instruction_.flags.Rs);

    switch (instruction_.flags.Op)
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
            ++cycles;
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
                carryOut = (op1 & (0x8000'0000 >> (op2 - 1)));
                result <<= op2;
            }

            break;
        }
        case 0b0011:  // LSR
        {
            op2 &= 0xFF;
            result = op1;
            ++cycles;
            updateOverflow = false;

            if (op2 > 32)
            {
                carryOut = false;
                result = 0;
            }
            else if (op2 == 32)
            {
                carryOut = (op1 & 0x8000'0000);
                result = 0;
            }
            else if (op2 != 0)
            {
                carryOut = (op1 & (0x01 << (op2 - 1)));
                result >>= op2;
            }

            break;
        }
        case 0b0100:  // ASR
        {
            op2 &= 0xFF;
            result = op1;
            ++cycles;
            updateOverflow = false;

            bool msbSet = op1 & 0x8000'0000;

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
                    result |= (msbSet ? 0x8000'0000 : 0);
                    --op2;
                }
            }

            break;
        }
        case 0b0101:  // ADC
        {
            std::tie(carryOut, overflowOut) = Add32(op1, op2, result, carryOut);
            break;
        }
        case 0b0110:  // SBC
            std::tie(carryOut, overflowOut) = Sub32(op1, op2, result, carryOut);
            break;
        case 0b0111:  // ROR
        {
            op2 &= 0xFF;
            result = op1;
            ++cycles;
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
        {
            std::tie(carryOut, overflowOut) = Add32(op1, op2, result);
            storeResult = false;
            break;
        }
        case 0b1100:  // ORR
            result = op1 | op2;
            updateCarry = false;
            updateOverflow = false;
            break;
        case 0b1101:  // MUL
            result = op1 * op2;
            updateOverflow = false;
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

    cpu.registers_.SetNegative(result & 0x8000'0000);
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
        cpu.registers_.WriteRegister(instruction_.flags.Rd, result);
    }

    return cycles;
}

int MoveCompareAddSubtractImmediate::Execute(ARM7TDMI& cpu)
{
    if (Config::LOGGING_ENABLED)
    {
        SetMnemonic();
    }

    bool carryOut = cpu.registers_.IsCarry();
    bool overflowOut = cpu.registers_.IsOverflow();
    bool saveResult = true;
    bool updateAllFlags = true;
    uint32_t op1 = cpu.registers_.ReadRegister(instruction_.flags.Rd);
    uint32_t op2 = instruction_.flags.Offset8;
    uint32_t result;

    switch (instruction_.flags.Op)
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

    cpu.registers_.SetNegative(result & 0x8000'0000);
    cpu.registers_.SetZero(result == 0);

    if (updateAllFlags)
    {
        cpu.registers_.SetCarry(carryOut);
        cpu.registers_.SetOverflow(overflowOut);
    }

    if (saveResult)
    {
        cpu.registers_.WriteRegister(instruction_.flags.Rd, result);
    }

    return 1;
}

int AddSubtract::Execute(ARM7TDMI& cpu)
{
    if (Config::LOGGING_ENABLED)
    {
        SetMnemonic();
    }

    uint32_t op1 = cpu.registers_.ReadRegister(instruction_.flags.Rs);
    uint32_t op2 = instruction_.flags.I ? instruction_.flags.RnOffset3 : cpu.registers_.ReadRegister(instruction_.flags.RnOffset3);
    bool carryOut;
    bool overflowOut;
    uint32_t result;

    if (instruction_.flags.Op)
    {
        std::tie(carryOut, overflowOut) = Sub32(op1, op2, result);
    }
    else
    {
        std::tie(carryOut, overflowOut) = Add32(op1, op2, result);
    }

    cpu.registers_.SetNegative(result & 0x8000'0000);
    cpu.registers_.SetZero(result == 0);
    cpu.registers_.SetCarry(carryOut);
    cpu.registers_.SetOverflow(overflowOut);
    cpu.registers_.WriteRegister(instruction_.flags.Rd, result);

    return 1;
}

int MoveShiftedRegister::Execute(ARM7TDMI& cpu)
{
    if (Config::LOGGING_ENABLED)
    {
        SetMnemonic();
    }

    bool carryOut = cpu.registers_.IsCarry();

    uint32_t result;
    uint32_t operand = cpu.registers_.ReadRegister(instruction_.flags.Rs);
    uint8_t shiftAmount = instruction_.flags.Offset5;

    switch (instruction_.flags.Op)
    {
        case 0b00:  // LSL
        {
            if (shiftAmount == 0)
            {
                result = operand;
            }
            else
            {
                carryOut = (operand & (0x8000'0000 >> (shiftAmount - 1)));
                result = operand << shiftAmount;
            }
            break;
        }
        case 0b01:  // LSR
        {
            if (shiftAmount == 0)
            {
                // LSR #0 -> LSR #32
                carryOut = (operand & 0x8000'0000);
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
            bool msbSet = operand & 0x8000'0000;

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
                    operand |= (msbSet ? 0x8000'0000 : 0);
                }

                result = operand;
            }
            break;
        }
    }

    cpu.registers_.SetNegative(result & 0x8000'0000);
    cpu.registers_.SetZero(result == 0);
    cpu.registers_.SetCarry(carryOut);
    cpu.registers_.WriteRegister(instruction_.flags.Rd, result);

    return 1;
}
}
