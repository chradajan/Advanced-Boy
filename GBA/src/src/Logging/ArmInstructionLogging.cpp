#include <ARM7TDMI/ArmInstructions.hpp>
#include <ARM7TDMI/ARM7TDMI.hpp>
#include <Config.hpp>
#include <format>
#include <sstream>
#include <string>

namespace
{
/// @brief Convert an ARM condition into its mnemonic.
/// @return ARM condition mnemonic.
std::string ConditionMnemonic(uint8_t condition)
{
    switch (condition)
    {
        case 0:
            return "EQ";
        case 1:
            return "NE";
        case 2:
            return "CS";
        case 3:
            return "CC";
        case 4:
            return "MI";
        case 5:
            return "PL";
        case 6:
            return "VS";
        case 7:
            return "VC";
        case 8:
            return "HI";
        case 9:
            return "LS";
        case 10:
            return "GE";
        case 11:
            return "LT";
        case 12:
            return "GT";
        case 13:
            return "LE";
        case 14:
            return "";
        default:
            return "";
    }
}

/// @brief Help write LDM/STM formatted register string.
/// @param regStream Stringstream to write to.
/// @param consecutiveRegisters How many consecutive registers were included in the operation.
/// @param regIndex Current index of register not included in operation.
void BlockDataTransferHelper(std::stringstream& regStream, int consecutiveRegisters, uint8_t regIndex)
{
    if (consecutiveRegisters <= 3)
    {
        for (int r = regIndex - consecutiveRegisters; r < regIndex; ++r)
        {
            regStream << "R" << r << ",";
        }
    }
    else
    {
        regStream << "R" << (regIndex - consecutiveRegisters) << "-R" << (regIndex - 1) << ",";
    }
}
}

namespace CPU::ARM
{
void BranchAndExchange::SetMnemonic()
{

}

void BlockDataTransfer::SetMnemonic()
{
    std::string op;
    std::string cond = ConditionMnemonic(instruction_.flags.Cond);
    uint8_t const addrReg = instruction_.flags.Rn;
    bool isStackOp = addrReg == 13;

    std::string addr = isStackOp ? "SP" : std::format("R{}", addrReg);

    uint8_t addressingModeCase =
        (instruction_.flags.L ? 0b100 : 0) | (instruction_.flags.P ? 0b010 : 0) | (instruction_.flags.U ? 0b001 : 0);

    switch (addressingModeCase)
    {
        case 0:
            op = isStackOp ? "STMED" : "STMDA";
            break;
        case 1:
            op = isStackOp ? "STMEA" : "STMIA";
            break;
        case 2:
            op = isStackOp ? "STMFD" : "STMDB";
            break;
        case 3:
            op = isStackOp ? "STMFA" : "STMIB";
            break;
        case 4:
            op = isStackOp ? "LDMFA" : "LDMDA";
            break;
        case 5:
            op = isStackOp ? "LDMFD" : "LDMIA";
            break;
        case 6:
            op = isStackOp ? "LDMEA" : "LDMDB";
            break;
        case 7:
            op = isStackOp ? "LDMED" : "LDMIB";
            break;
    }

    uint8_t regIndex = 0;
    std::stringstream regStream;
    uint16_t regList = instruction_.flags.RegisterList;
    int consecutiveRegisters = 0;

    regStream << "{";

    while (regList != 0)
    {
        if (regList & 0x01)
        {
            ++consecutiveRegisters;
        }
        else if (consecutiveRegisters > 0)
        {
            BlockDataTransferHelper(regStream, consecutiveRegisters, regIndex);
            consecutiveRegisters = 0;
        }

        ++regIndex;
        regList >>= 1;
    }

    if (consecutiveRegisters > 0)
    {
        BlockDataTransferHelper(regStream, consecutiveRegisters, regIndex);
    }

    if (regStream.str().length() > 1)
    {
        regStream.seekp(-1, regStream.cur);
    }

    regStream << "}";

    mnemonic_ = std::format("{:08X} -> {}{} {}{}, {}{}",
        instruction_.word, op, cond, addr, instruction_.flags.W ? "!" : "", regStream.str(), instruction_.flags.S ? "^" : "");
}

void Branch::SetMnemonic(uint32_t newPC)
{
    std::string op = instruction_.flags.L ? "BL" : "B";
    std::string cond = ConditionMnemonic(instruction_.flags.Cond);
    mnemonic_ = std::format("{:08X} -> {}{} {:08X}", instruction_.word, op, cond, newPC);
}

void SoftwareInterrupt::SetMnemonic()
{

}

void Undefined::SetMnemonic()
{

}

void SingleDataTransfer::SetMnemonic()
{

}

void SingleDataSwap::SetMnemonic()
{

}

void Multiply::SetMnemonic()
{

}

void MultiplyLong::SetMnemonic()
{

}

void HalfwordDataTransferRegisterOffset::SetMnemonic()
{

}

void HalfwordDataTransferImmediateOffset::SetMnemonic()
{

}

void PSRTransferMRS::SetMnemonic()
{

}

void PSRTransferMSR::SetMnemonic()
{

}

void DataProcessing::SetMnemonic()
{

}
}
