#include <ARM7TDMI/ThumbInstructions.hpp>
#include <ARM7TDMI/ARM7TDMI.hpp>
#include <cstdint>
#include <memory>

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

void SoftwareInterrupt::Execute(ARM7TDMI* const cpu)
{

}

void UnconditionalBranch::Execute(ARM7TDMI* const cpu)
{

}

void ConditionalBranch::Execute(ARM7TDMI* const cpu)
{

}

void MultipleLoadStore::Execute(ARM7TDMI* const cpu)
{

}

void LongBranchWithLink::Execute(ARM7TDMI* const cpu)
{

}

void AddOffsetToStackPointer::Execute(ARM7TDMI* const cpu)
{

}

void PushPopRegisters::Execute(ARM7TDMI* const cpu)
{

}

void LoadStoreHalfword::Execute(ARM7TDMI* const cpu)
{

}

void SPRelativeLoadStore::Execute(ARM7TDMI* const cpu)
{

}

void LoadAddress::Execute(ARM7TDMI* const cpu)
{

}

void LoadStoreWithImmediateOffset::Execute(ARM7TDMI* const cpu)
{

}

void LoadStoreWithRegisterOffset::Execute(ARM7TDMI* const cpu)
{

}

void LoadStoreSignExtendedByteHalfword::Execute(ARM7TDMI* const cpu)
{

}

void PCRelativeLoad::Execute(ARM7TDMI* const cpu)
{

}

void HiRegisterOperationsBranchExchange::Execute(ARM7TDMI* const cpu)
{

}

void ALUOperations::Execute(ARM7TDMI* const cpu)
{

}

void MoveCompareAddSubtractImmediate::Execute(ARM7TDMI* const cpu)
{

}

void AddSubtract::Execute(ARM7TDMI* const cpu)
{

}

void MoveShiftedRegister::Execute(ARM7TDMI* const cpu)
{
    
}
}
