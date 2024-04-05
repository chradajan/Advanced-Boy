#include <ARM7TDMI/ThumbInstructions.hpp>
#include <ARM7TDMI/ARM7TDMI.hpp>
#include <Logging/Logging.hpp>
#include <format>
#include <string>

namespace CPU::THUMB
{
void SoftwareInterrupt::SetMnemonic()
{

}

void UnconditionalBranch::SetMnemonic(uint32_t newPC)
{
    mnemonic_ = std::format("{:04X} -> B #{:08X}", instruction_.halfword, newPC);
}

void ConditionalBranch::SetMnemonic(uint32_t newPC)
{
    std::string condition = Logging::ConditionMnemonic(instruction_.flags.Cond);
    mnemonic_ = std::format("{:04X} -> B{} 0x{:08X}", instruction_.halfword, condition, newPC);
}

void MultipleLoadStore::SetMnemonic()
{

}

void LongBranchWithLink::SetMnemonic()
{

}

void AddOffsetToStackPointer::SetMnemonic()
{

}

void PushPopRegisters::SetMnemonic()
{

}

void LoadStoreHalfword::SetMnemonic()
{

}

void SPRelativeLoadStore::SetMnemonic()
{

}

void LoadAddress::SetMnemonic()
{

}

void LoadStoreWithImmediateOffset::SetMnemonic()
{

}

void LoadStoreWithRegisterOffset::SetMnemonic()
{

}

void LoadStoreSignExtendedByteHalfword::SetMnemonic()
{

}

void HiRegisterOperationsBranchExchange::SetMnemonic()
{

}

void PCRelativeLoad::SetMnemonic()
{

}

void ALUOperations::SetMnemonic()
{

}

void MoveCompareAddSubtractImmediate::SetMnemonic()
{
    std::string op;

    switch (instruction_.flags.Op)
    {
        case 0b00:
            op = "MOV";
            break;
        case 0b01:
            op = "CMP";
            break;
        case 0b10:
            op = "ADD";
            break;
        case 0b11:
            op = "SUB";
            break;
    }

    uint8_t destIndex = instruction_.flags.Rd;
    uint8_t offset = instruction_.flags.Offset8;

    mnemonic_ = std::format("{:04X} -> {} R{}, #{}", instruction_.halfword, op, destIndex, offset);
}

void AddSubtract::SetMnemonic()
{

}

void MoveShiftedRegister::SetMnemonic()
{
    std::string op;

    switch (instruction_.flags.Op)
    {
        case 0b00:
            op = "LSL";
            break;
        case 0b01:
            op = "LSR";
            break;
        case 0b10:
            op = "ASR";
            break;
    }

    uint8_t destIndex = instruction_.flags.Rd;
    uint8_t srcIndex = instruction_.flags.Rs;
    uint8_t offset = instruction_.flags.Offset5;

    mnemonic_ = std::format("{:04X} -> {} R{}, R{}, #{}", instruction_.halfword, op, destIndex, srcIndex, offset);
}
}
