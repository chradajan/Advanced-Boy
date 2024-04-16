#include <ARM7TDMI/ThumbInstructions.hpp>
#include <ARM7TDMI/ARM7TDMI.hpp>
#include <Logging/Logging.hpp>
#include <format>
#include <sstream>
#include <string>

namespace
{
/// @brief Help write Push/Pop formatted register string.
/// @param regStream Stringstream to write to.
/// @param consecutiveRegisters How many consecutive registers were included in the operation.
/// @param regIndex Current index of register not included in operation.
void PushPopHelper(std::stringstream& regStream, int consecutiveRegisters, uint8_t regIndex)
{
    if (consecutiveRegisters <= 2)
    {
        for (int r = regIndex - consecutiveRegisters; r < regIndex; ++r)
        {
            regStream << "R" << r << ", ";
        }
    }
    else
    {
        regStream << "R" << (regIndex - consecutiveRegisters) << "-R" << (regIndex - 1) << ", ";
    }
}
}

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

void LongBranchWithLink::SetMnemonic(uint32_t newPC)
{
    if (!instruction_.flags.H)
    {
        // Instruction 1
        mnemonic_ = std::format("{:04X} -> BL", instruction_.halfword);
    }
    else
    {
        // Instruction 2
        mnemonic_ = std::format("{:04X} -> BL 0x{:08X}", instruction_.halfword, newPC);
    }
}

void AddOffsetToStackPointer::SetMnemonic(uint16_t offset)
{
    std::string imm = std::format("#{}{}", instruction_.flags.S ? "-" : "", offset);
    mnemonic_ = std::format("{:04X} -> ADD SP, {}", instruction_.halfword, imm);
}

void PushPopRegisters::SetMnemonic()
{
    std::string op = instruction_.flags.L ? "POP" : "PUSH";
    std::string r = "";

    if (instruction_.flags.R)
    {
        r = instruction_.flags.L ? "PC" : "LR";
    }

    uint8_t regIndex = 0;
    std::stringstream regStream;
    uint8_t regList = instruction_.flags.Rlist;
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
            PushPopHelper(regStream, consecutiveRegisters, regIndex);
            consecutiveRegisters = 0;
        }

        ++regIndex;
        regList >>= 1;
    }

    if (consecutiveRegisters > 0)
    {
        PushPopHelper(regStream, consecutiveRegisters, regIndex);
    }

    if (regStream.str().length() > 1)
    {
        if (r != "")
        {
            regStream << r;
        }
        else
        {
            regStream.seekp(-2, regStream.cur);
        }
    }
    else
    {
        regStream << r;
    }

    regStream << "}";
    mnemonic_ = std::format("{:04X} -> {} {}", instruction_.halfword, op, regStream.str());
}

void LoadStoreHalfword::SetMnemonic()
{
    uint8_t srcDestIndex = instruction_.flags.Rd;
    uint8_t baseIndex = instruction_.flags.Rb;
    uint8_t offset = instruction_.flags.Offset5 << 1;

    std::string op = instruction_.flags.L ? "LDRH" : "STRH";
    mnemonic_ = std::format("{:04X} -> {} R{}, [R{}, #{}]", instruction_.halfword, op, srcDestIndex, baseIndex, offset);
}

void SPRelativeLoadStore::SetMnemonic()
{
    std::string op = instruction_.flags.L ? "LDR" : "STR";
    uint8_t rd = instruction_.flags.Rd;
    uint16_t imm = instruction_.flags.Word8 << 2;
    mnemonic_ = std::format("{:04X} -> {} R{}, [SP, #{}]",instruction_.halfword, op, rd, imm);
}

void LoadAddress::SetMnemonic(uint8_t destIndex, uint16_t offset)
{
    std::string reg = instruction_.flags.SP ? "SP" : "PC";
    mnemonic_ = std::format("{:04X} -> ADD R{}, {}, #{}", instruction_.halfword, destIndex, reg, offset);
}

void LoadStoreWithImmediateOffset::SetMnemonic()
{
    std::string op = instruction_.flags.L ? "LDR" : "STR";
    std::string b = instruction_.flags.B ? "B" : "";
    op = op + b;

    uint8_t rb = instruction_.flags.Rb;
    uint8_t rd = instruction_.flags.Rd;
    uint8_t imm = (instruction_.flags.B) ? instruction_.flags.Offset5 : (instruction_.flags.Offset5 << 2);

    mnemonic_ = std::format("{:04X} -> {} R{}, [R{}, #{}]", instruction_.halfword, op, rd, rb, imm);
}

void LoadStoreWithRegisterOffset::SetMnemonic()
{
    std::string op = instruction_.flags.L ? "LDR" : "STR";
    std::string b = instruction_.flags.B ? "B" : "";
    op = op + b;

    uint8_t ro = instruction_.flags.Ro;
    uint8_t rb = instruction_.flags.Rb;
    uint8_t rd = instruction_.flags.Rd;

    mnemonic_ = std::format("{:04X} -> {} R{}, [R{}, R{}]", instruction_.halfword, op, rd, rb, ro);
}

void LoadStoreSignExtendedByteHalfword::SetMnemonic()
{
    uint8_t ro = instruction_.flags.Ro;
    uint8_t rb = instruction_.flags.Rb;
    uint8_t rd = instruction_.flags.Rd;
    std::string regString = std::format("R{}, [R{}, R{}]", rd, rb, ro);

    std::string op;
    uint8_t sh = (instruction_.flags.S << 1) | (instruction_.flags.H);

    switch (sh)
    {
        case 0b00:  // S = 0, H = 0
            op = "STRH";
            break;
        case 0b01:  // S = 0, H = 1
            op = "LDRH";
            break;
        case 0b10:  // S = 1, H = 0
            op = "LDSB";
            break;
        case 0b11:  // S = 1, H = 1
            op = "LDSH";
            break;
    }

    mnemonic_ = std::format("{:04X} -> {} {}", instruction_.halfword, op, regString);
}

void HiRegisterOperationsBranchExchange::SetMnemonic(uint8_t destIndex, uint8_t srcIndex)
{
    std::string op;
    std::string regString = std::format("R{}, R{}", destIndex, srcIndex);

    switch (instruction_.flags.Op)
    {
        case 0b00:
            op = "ADD";
            break;
        case 0b01:
            op = "CMP";
            break;
        case 0b10:
            op = "MOV";
            break;
        case 0b11:
            op = "BX";
            regString = std::format("R{}", srcIndex);
            break;
    }

    mnemonic_ = std::format("{:04X} -> {} {}", instruction_.halfword, op, regString);
}

void PCRelativeLoad::SetMnemonic()
{
    uint8_t destIndex = instruction_.flags.Rd;
    uint16_t offset = instruction_.flags.Word8 << 2;
    mnemonic_ = std::format("{:04X} -> LDR R{}, [PC, #{}]",instruction_.halfword, destIndex, offset);
}

void ALUOperations::SetMnemonic()
{
    std::string op;

    switch (instruction_.flags.Op)
    {
        case 0b0000:
            op = "AND";
            break;
        case 0b0001:
            op = "EOR";
            break;
        case 0b0010:
            op = "LSL";
            break;
        case 0b0011:
            op = "LSR";
            break;
        case 0b0100:
            op = "ASR";
            break;
        case 0b0101:
            op = "ADC";
            break;
        case 0b0110:
            op = "SBC";
            break;
        case 0b0111:
            op = "ROR";
            break;
        case 0b1000:
            op = "TST";
            break;
        case 0b1001:
            op = "NEG";
            break;
        case 0b1010:
            op = "CMP";
            break;
        case 0b1011:
            op = "CMN";
            break;
        case 0b1100:
            op = "ORR";
            break;
        case 0b1101:
            op = "MUL";
            break;
        case 0b1110:
            op = "BIC";
            break;
        case 0b1111:
            op = "MVN";
            break;
    }

    uint8_t destIndex = instruction_.flags.Rd;
    uint8_t srcIndex = instruction_.flags.Rs;

    mnemonic_ = std::format("{:04X} -> {} R{}, R{}", instruction_.halfword, op, destIndex, srcIndex);
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
    std::string op;
    uint8_t rnOffset = instruction_.flags.RnOffset3;

    if (instruction_.flags.I && (rnOffset == 0))
    {
        op = "MOV";
    }
    else
    {
        op = instruction_.flags.Op ? "SUB" : "ADD";
    }

    std::string operand = "";

    if (!instruction_.flags.I)
    {
        operand = std::format(", R{}", rnOffset);
    }
    else if (rnOffset > 0)
    {
        operand = std::format(", #{}", rnOffset);
    }

    uint8_t destIndex = instruction_.flags.Rd;
    uint8_t srcIndex = instruction_.flags.Rs;

    mnemonic_ = std::format("{:04X} -> {} R{}, R{}{}", instruction_.halfword, op, destIndex, srcIndex, operand);
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
