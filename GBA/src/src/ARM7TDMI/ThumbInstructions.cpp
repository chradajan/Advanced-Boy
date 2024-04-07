#include <ARM7TDMI/ThumbInstructions.hpp>
#include <ARM7TDMI/ARM7TDMI.hpp>
#include <Config.hpp>
#include <bit>
#include <cstdint>
#include <memory>
#include <stdexcept>

namespace
{
/// @brief Determine the result of the overflow flag for addition operation: result = op1 + op2
/// @param op1 Addition operand
/// @param op2 Addition operand
/// @param result Addition result
/// @return State of overflow flag after addition
bool OverflowFlagAddition(uint32_t op1, uint32_t op2, uint32_t result)
{
    return (~(op1 ^ op2) & ((op1 ^ result)) & 0x8000'0000) != 0;
}

/// @brief Determine the result of the overflow flag for subtraction operation: result = op1 - op2
/// @param op1 Subtraction operand
/// @param op2 Subtraction operand
/// @param result Subtraction result
/// @return State of overflow flag after subtraction
bool OverflowFlagSubtraction(uint32_t op1, uint32_t op2, uint32_t result)
{
    return ((op1 ^ op2) & ((op1 ^ result)) & 0x8000'0000) != 0;
}

/// @brief Determine the result of the carry flag for addition operation.
/// @param result Un-truncated sum from addition operation.
/// @return State of carry flag after addition
bool CarryFlagAddition(uint64_t result)
{
    return result & 0x1'0000'0000;
}

/// @brief Determine the result of the carry flag for subtraction operation: op1 - op2
/// @param op1 Subtraction operand
/// @param op2 Subtraction operand
/// @return State of carry flag after subtraction
bool CarryFlagSubtraction(uint32_t op1, uint32_t op2)
{
    return op1 >= op2;
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

    if constexpr (Config::LOGGING_ENABLED)
    {
        SetMnemonic(newPC);
    }

    cpu.registers_.SetPC(newPC);
    cpu.flushPipeline_ = true;

    return 1;
}

int ConditionalBranch::Execute(ARM7TDMI& cpu)
{
    int cycles = 1;

    int16_t signedOffset = instruction_.flags.SOffset8 << 1;

    if (signedOffset & 0x0100)
    {
        signedOffset |= 0xFE00;
    }

    uint32_t newPC = cpu.registers_.GetPC() + signedOffset;

    if constexpr (Config::LOGGING_ENABLED)
    {
        SetMnemonic(newPC);
    }

    if (cpu.ArmConditionMet(instruction_.flags.Cond))
    {
        cpu.registers_.SetPC(newPC);
        cpu.flushPipeline_ = true;
    }

    return cycles;
}

int MultipleLoadStore::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: THUMB_MultipleLoadStore");
}

int LongBranchWithLink::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: THUMB_LongBranchWithLink");
}

int AddOffsetToStackPointer::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: THUMB_AddOffsetToStackPointer");
}

int PushPopRegisters::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: THUMB_PushPopRegisters");
}

int LoadStoreHalfword::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: THUMB_LoadStoreHalfword");
}

int SPRelativeLoadStore::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: THUMB_SPRelativeLoadStore");
}

int LoadAddress::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: THUMB_LoadAddress");
}

int LoadStoreWithImmediateOffset::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: THUMB_LoadStoreWithImmediateOffset");
}

int LoadStoreWithRegisterOffset::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: THUMB_LoadStoreWithRegisterOffset");
}

int LoadStoreSignExtendedByteHalfword::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: THUMB_LoadStoreSignExtendedByteHalfword");
}

int PCRelativeLoad::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: THUMB_PCRelativeLoad");
}

int HiRegisterOperationsBranchExchange::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: THUMB_HiRegisterOperationsBranchExchange");
}

int ALUOperations::Execute(ARM7TDMI& cpu)
{
    int cycles = 1;

    if constexpr (Config::LOGGING_ENABLED)
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
            ++cycles;
            updateOverflow = false;

            if (op2 > 32)
            {
                carryOut = false;
                result = 0;
            }
            else if (op2 != 0)
            {
                carryOut = (op1 & (0x8000'0000 >> (op2 - 1)));
                result = op1 << op2;
            }

            break;
        }
        case 0b0011:  // LSR
        {
            op2 &= 0xFF;
            ++cycles;
            updateOverflow = false;

            if (op2 > 32)
            {
                carryOut = false;
                result = 0;
            }
            else if (op2 != 0)
            {
                carryOut = (op1 & (0x01 << (op2 - 1)));
                result = op1 >> op2;
            }

            break;
        }
        case 0b0100:  // ASR
        {
            op2 &= 0xFF;
            ++cycles;
            updateOverflow = false;

            bool msbSet = op1 & 0x8000'0000;

            if (op2 > 32)
            {
                carryOut = msbSet;
                result = msbSet ? 0xFFFF'FFFF : 0;
            }
            else
            {
                carryOut = (op1 & (0x01 << (op2 - 1)));

                while (op2 > 0)
                {
                    op1 >>= 1;
                    op1 |= (msbSet ? 0x8000'0000 : 0);
                    --op2;
                }

                result = op1;
            }

            break;
        }
        case 0b0101:  // ADC
        {
            uint64_t result64 = op1 + op2 + (carryOut ? 1 : 0);
            result = result64 & MAX_U32;
            carryOut = CarryFlagAddition(result64);
            overflowOut = OverflowFlagAddition(op1, op2, result);
            break;
        }
        case 0b0110:  // SBC
            result = op1 - op2 - (carryOut ? 0 : 1);
            carryOut = CarryFlagSubtraction(op1, op2);
            overflowOut = OverflowFlagSubtraction(op1, op2, result);
            break;
        case 0b0111:  // ROR
        {
            op2 &= 0xFF;
            ++cycles;
            updateOverflow = false;

            if (op2 > 32)
            {
                op2 %= 32;
            }

            if (op2)
            {
                carryOut = (op1 & (0x01 << (op2 - 1)));
                result = std::rotr(op1, op2);
            }
            else
            {
                result = op1;
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
            result = 0 - op2;
            carryOut = CarryFlagSubtraction(0, op2);
            overflowOut = OverflowFlagSubtraction(0, op2, result);
            break;
        case 0b1010:  // CMP
            result = op1 - op2;
            carryOut = CarryFlagSubtraction(op1, op2);
            overflowOut = OverflowFlagSubtraction(op1, op2, result);
            storeResult = false;
            break;
        case 0b1011:  // CMN
        {
            uint64_t result64 = op1 + op2;
            result = result64 & MAX_U32;
            carryOut = CarryFlagAddition(result64);
            overflowOut = OverflowFlagAddition(op1, op2, result);
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
    if constexpr (Config::LOGGING_ENABLED)
    {
        SetMnemonic();
    }

    bool carryOut = cpu.registers_.IsCarry();
    bool overflowOut = cpu.registers_.IsOverflow();
    bool saveResult = true;
    bool updateAllFlags = true;
    uint32_t op1 = cpu.registers_.ReadRegister(instruction_.flags.Rd);
    uint32_t op2 = instruction_.flags.Offset8;
    uint64_t result;

    switch (instruction_.flags.Op)
    {
        case 0b00:  // MOV
            result = op2;
            updateAllFlags = false;
            break;
        case 0b01:  // CMP
            result = op1 - op2;
            result &= CPU::MAX_U32;
            overflowOut = OverflowFlagSubtraction(op1, op2, result);
            carryOut = CarryFlagSubtraction(op1, op2);
            saveResult = false;
            break;
        case 0b10:  // ADD
            result = op1 + op2;
            carryOut = CarryFlagAddition(result);
            result &= CPU::MAX_U32;
            overflowOut = OverflowFlagAddition(op1, op2, result);
            break;
        case 0b11:  // SUB
            result = op1 - op2;
            result &= CPU::MAX_U32;
            carryOut = CarryFlagSubtraction(op1, op2);
            overflowOut = OverflowFlagSubtraction(op1, op2, result);
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
    if constexpr (Config::LOGGING_ENABLED)
    {
        SetMnemonic();
    }

    uint32_t op1 = cpu.registers_.ReadRegister(instruction_.flags.Rs);
    uint32_t op2 = instruction_.flags.I ? instruction_.flags.RnOffset3 : cpu.registers_.ReadRegister(instruction_.flags.RnOffset3);
    uint64_t result64;
    uint32_t result;

    if (instruction_.flags.Op)
    {
        result64 = op1 - op2;
        result = result64 & MAX_U32;
        cpu.registers_.SetCarry(CarryFlagSubtraction(op1, op2));
        cpu.registers_.SetOverflow(OverflowFlagSubtraction(op1, op2, result));
    }
    else
    {
        result64 = op1 + op2;
        result = result64 & MAX_U32;
        cpu.registers_.SetCarry(CarryFlagAddition(result64));
        cpu.registers_.SetOverflow(OverflowFlagAddition(op1, op2, result));
    }

    cpu.registers_.SetNegative(result & 0x8000'0000);
    cpu.registers_.SetZero(result == 0);
    cpu.registers_.WriteRegister(instruction_.flags.Rd, result);

    return 1;
}

int MoveShiftedRegister::Execute(ARM7TDMI& cpu)
{
    if constexpr (Config::LOGGING_ENABLED)
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
