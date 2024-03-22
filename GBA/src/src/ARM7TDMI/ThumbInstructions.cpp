#include <ARM7TDMI/ThumbInstructions.hpp>
#include <ARM7TDMI/ARM7TDMI.hpp>
#include <cstdint>
#include <memory>
#include <stdexcept>

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

void SoftwareInterrupt::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: THUMB_SoftwareInterrupt");
}

SoftwareInterrupt::operator std::string() const
{
    return "";
}

void UnconditionalBranch::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: THUMB_UnconditionalBranch");
}

UnconditionalBranch::operator std::string() const
{
    return "";
}

void ConditionalBranch::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: THUMB_ConditionalBranch");
}

ConditionalBranch::operator std::string() const
{
    return "";
}

void MultipleLoadStore::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: THUMB_MultipleLoadStore");
}

MultipleLoadStore::operator std::string() const
{
    return "";
}

void LongBranchWithLink::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: THUMB_LongBranchWithLink");
}

LongBranchWithLink::operator std::string() const
{
    return "";
}

void AddOffsetToStackPointer::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: THUMB_AddOffsetToStackPointer");
}

AddOffsetToStackPointer::operator std::string() const
{
    return "";
}

void PushPopRegisters::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: THUMB_PushPopRegisters");
}

PushPopRegisters::operator std::string() const
{
    return "";
}

void LoadStoreHalfword::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: THUMB_LoadStoreHalfword");
}

LoadStoreHalfword::operator std::string() const
{
    return "";
}

void SPRelativeLoadStore::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: THUMB_SPRelativeLoadStore");
}

SPRelativeLoadStore::operator std::string() const
{
    return "";
}

void LoadAddress::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: THUMB_LoadAddress");
}

LoadAddress::operator std::string() const
{
    return "";
}

void LoadStoreWithImmediateOffset::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: THUMB_LoadStoreWithImmediateOffset");
}

LoadStoreWithImmediateOffset::operator std::string() const
{
    return "";
}

void LoadStoreWithRegisterOffset::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: THUMB_LoadStoreWithRegisterOffset");
}

LoadStoreWithRegisterOffset::operator std::string() const
{
    return "";
}

void LoadStoreSignExtendedByteHalfword::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: THUMB_LoadStoreSignExtendedByteHalfword");
}

LoadStoreSignExtendedByteHalfword::operator std::string() const
{
    return "";
}

void PCRelativeLoad::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: THUMB_PCRelativeLoad");
}

PCRelativeLoad::operator std::string() const
{
    return "";
}

void HiRegisterOperationsBranchExchange::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: THUMB_HiRegisterOperationsBranchExchange");
}

HiRegisterOperationsBranchExchange::operator std::string() const
{
    return "";
}

void ALUOperations::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: THUMB_ALUOperations");
}

ALUOperations::operator std::string() const
{
    return "";
}

void MoveCompareAddSubtractImmediate::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: THUMB_MoveCompareAddSubtractImmediate");
}

MoveCompareAddSubtractImmediate::operator std::string() const
{
    return "";
}

void AddSubtract::Execute(ARM7TDMI& cpu)
{
    //  OP | I | OpCode
    // ----------------
    //  0  | 0 | ADD Rd, Rs, Rn
    //  0  | 1 | ADD Rd, Rs, #Offset3
    //  1  | 0 | SUB Rd, Rs, Rn
    //  1  | 1 | SUB Rd, Rs, #Offset3

    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: THUMB_AddSubtract");

    // uint64_t result = 0;
    // uint32_t source = cpu.registers_.ReadRegister(instruction_.flags.Rs);
    // uint32_t operand =
    //     instruction_.flags.I ? instruction_.flags.RnOffset3 : cpu.registers_.ReadRegister(instruction_.flags.RnOffset3);

    // if (instruction_.flags.Op)
    // {
    //     // SUB

    // }
    // else
    // {
    //     // ADD
    //     result = source + operand;
    // }

    // uint32_t truncatedResult = result & MAX_32;

    // cpu.registers_.SetNegative(false);
    // cpu.registers_.SetZero(truncatedResult == 0);
    // cpu.registers_.SetCarry(result > MAX_32);
    // cpu.registers_.SetOverflow(result > MAX_32);
    // cpu.registers_.WriteRegister(instruction_.flags.Rd, truncatedResult);
}

AddSubtract::operator std::string() const
{
    return "";
}

void MoveShiftedRegister::Execute(ARM7TDMI& cpu)
{
    // ARM has some weird shift behavior as described below:
    // LSL #0 -> no shift
    // LSR #0 -> LSR #32
    // ASR #0 -> ASR #32

    //  OP | OpCode
    // -------------
    //  00 | LSL Rd, Rs, #Offset5
    //  01 | LSR Rd, Rs, #Offset5
    //  10 | ASR Rd, Rs, #Offset5

    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: THUMB_MoveShiftedRegister");

    // uint32_t operand = cpu.registers_.ReadRegister(instruction_.flags.Rs);

    // switch (instruction_.flags.Op)
    // {
    //     case 0:  // LSL
    //         operand <<= instruction_.flags.Offset5;
    //         break;
    //     case 1:  // LSR
    //         operand >>= instruction_.flags.Offset5;
    //         break;
    //     case 2:  // ASR
    //         if (operand & MSB_32)
    //         {
    //             operand >>= instruction_.flags.Offset5;

    //         }
    //         else
    //         {
    //             operand >>= instruction_.flags.Offset5;
    //         }
    //         break;
    //     case 3:
    //         throw std::runtime_error("Invalid MoveShiftedRegister OpCode");
    // }

    // cpu.registers_.WriteRegister(instruction_.flags.Rd, operand);
}

MoveShiftedRegister::operator std::string() const
{
    return "";
}
}
