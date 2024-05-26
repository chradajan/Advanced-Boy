#include <ARM7TDMI/ArmInstructions.hpp>
#include <ARM7TDMI/ARM7TDMI.hpp>
#include <ARM7TDMI/Registers.hpp>
#include <Logging/Logging.hpp>
#include <bit>
#include <cstdint>
#include <format>
#include <sstream>
#include <string>

namespace
{
/// @brief Form a mnemonic for halfword data transfer ops.
/// @param load Whether this is a load or store op.
/// @param cond ARM condition code.
/// @param destIndex Index of destination register.
/// @param baseIndex Index of base register.
/// @param s S flag.
/// @param h H flag.
/// @param up Add or Subtract offset from base.
/// @param preIndexed Pre/Post indexed op.
/// @param writeBack Whether to write back to 
/// @param offsetExpression String of either immediate offset or offset register index.
/// @return Mnemonic of instruction.
std::string HalfwordDataTransferHelper(bool load,
                                       uint8_t cond,
                                       uint8_t destIndex,
                                       uint8_t baseIndex,
                                       bool s,
                                       bool h,
                                       bool up,
                                       bool preIndexed,
                                       bool writeBack,
                                       std::string offsetExpression)
{
    std::string op = load ? "LDR" : "STR";
    std::string opType;

    if (s)
    {
        opType = h ? "SH" : "SB";
    }
    else
    {
        opType = "H";
    }

    std::string address;

    if (offsetExpression == "")
    {
        address = std::format("[R{}]", baseIndex);
    }
    else
    {
        if (preIndexed)
        {
            address = std::format("[R{}, {}{}]{}", baseIndex, up ? "+" : "-", offsetExpression, writeBack ? "!" : "");
        }
        else
        {
            address = std::format("[R{}], {}{}", baseIndex, up ? "+" : "-", offsetExpression);
        }
    }

    return std::format("{}{}{} R{}, {}", op, opType, Logging::ConditionMnemonic(cond), destIndex, address);
}

/// @brief Help write LDM/STM formatted register string.
/// @param regStream Stringstream to write to.
/// @param consecutiveRegisters How many consecutive registers were included in the operation.
/// @param regIndex Current index of register not included in operation.
void BlockDataTransferHelper(std::stringstream& regStream, int consecutiveRegisters, uint8_t regIndex)
{
    if (consecutiveRegisters <= 2)
    {
        for (int r = regIndex - consecutiveRegisters; r < regIndex; ++r)
        {
            if (r == ::CPU::LR_INDEX)
            {
                regStream << "LR, ";
            }
            else if (r == ::CPU::PC_INDEX)
            {
                regStream << "PC, ";
            }
            else
            {
                regStream << "R" << r << ", ";
            }
        }
    }
    else
    {
        regStream << "R" << (regIndex - consecutiveRegisters) << "-R" << (regIndex - 1) << ", ";
    }
}
}

namespace CPU::ARM
{
void BranchAndExchange::SetMnemonic()
{
    std::string op = "BX";
    std::string cond = Logging::ConditionMnemonic(instruction_.flags.Cond);
    uint8_t operandRegIndex = instruction_.flags.Rn;

    mnemonic_ = std::format("{:08X} -> {}{} R{}", instruction_.word, op, cond, operandRegIndex);
}

void BlockDataTransfer::SetMnemonic()
{
    std::string op;
    std::string cond = Logging::ConditionMnemonic(instruction_.flags.Cond);
    uint8_t const addrReg = instruction_.flags.Rn;
    bool isStackOp = (addrReg == SP_INDEX);

    std::string addr = isStackOp ? "SP" : std::format("R{}", addrReg);

    uint8_t addressingModeCase =
        (instruction_.flags.L ? 0b100 : 0) | (instruction_.flags.P ? 0b010 : 0) | (instruction_.flags.U ? 0b001 : 0);

    switch (addressingModeCase)
    {
        case 0b000:
            op = isStackOp ? "STMED" : "STMDA";
            break;
        case 0b001:
            op = isStackOp ? "STMEA" : "STMIA";
            break;
        case 0b010:
            op = isStackOp ? "STMFD" : "STMDB";
            break;
        case 0b011:
            op = isStackOp ? "STMFA" : "STMIB";
            break;
        case 0b100:
            op = isStackOp ? "LDMFA" : "LDMDA";
            break;
        case 0b101:
            op = isStackOp ? "LDMFD" : "LDMIA";
            break;
        case 0b110:
            op = isStackOp ? "LDMEA" : "LDMDB";
            break;
        case 0b111:
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
        regStream.seekp(-2, regStream.cur);
    }

    regStream << "}";

    mnemonic_ = std::format("{:08X} -> {}{} {}{}, {}{}",
        instruction_.word, op, cond, addr, instruction_.flags.W ? "!" : "", regStream.str(), instruction_.flags.S ? "^" : "");
}

void Branch::SetMnemonic(uint32_t newPC)
{
    std::string op = instruction_.flags.L ? "BL" : "B";
    std::string cond = Logging::ConditionMnemonic(instruction_.flags.Cond);
    mnemonic_ = std::format("{:08X} -> {}{} 0x{:08X}", instruction_.word, op, cond, newPC);
}

void SoftwareInterrupt::SetMnemonic()
{
    std::string cond = Logging::ConditionMnemonic(instruction_.flags.Cond);
    uint32_t comment = instruction_.flags.CommentField;
    mnemonic_ = std::format("{:08X} -> SWI{} #{:06X}", instruction_.word, cond, comment);
}

void Undefined::SetMnemonic()
{
    std::string cond = Logging::ConditionMnemonic(instruction_.flags.Cond);
    mnemonic_ = std::format("{:08X} -> UNDEFINED {}", instruction_.word, cond);
}

void SingleDataTransfer::SetMnemonic(uint32_t offset)
{
    std::string op = instruction_.flags.L ? "LDR" : "STR";
    std::string cond = Logging::ConditionMnemonic(instruction_.flags.Cond);
    std::string xfer = instruction_.flags.B ? "B" : "";
    op = op + cond + xfer;
    uint8_t srcDestIndex = instruction_.flags.Rd;
    uint8_t baseIndex = instruction_.flags.Rn;
    std::string address;
    std::string expression;

    if (instruction_.flags.I)
    {
        std::string shiftExpression;
        std::string shiftType;
        uint8_t shiftRegIndex = instruction_.registerOffset.Rm;
        uint8_t shiftAmount = instruction_.registerOffset.ShiftAmount;

        // Shift operation
        switch (instruction_.registerOffset.ShiftType)
        {
            case 0b00:
                shiftType = "LSL";
                break;
            case 0b01:
                shiftType = "LSR";
                shiftAmount = (shiftAmount == 0) ? 32 : shiftAmount;
                break;
            case 0b10:
                shiftType = "ASR";
                shiftAmount = (shiftAmount == 0) ? 32 : shiftAmount;
                break;
            case 0b11:
                shiftType = (shiftAmount == 0) ? "RRX" : "ROR";
                break;
        }

        if (shiftType == "RRX")
        {
            shiftExpression = std::format("R{}, RRX", shiftRegIndex);
        }
        else if ((shiftType == "LSL") && (shiftAmount == 0))
        {
            shiftExpression = std::format("R{}", shiftRegIndex);
        }
        else
        {
            shiftExpression = std::format("R{}, {} #{}", shiftRegIndex, shiftType, shiftAmount);
        }

        expression = std::format("{}{}", instruction_.flags.U ? "+" : "-", shiftExpression);
    }
    else
    {
        if (offset == 0)
        {
            expression = "";
        }
        else
        {
            expression = std::format("#{}{}", instruction_.flags.U ? "+" : "-", offset);
        }
    }

    if (instruction_.flags.P)
    {
        // Pre-indexed
        if (expression == "")
        {
            address = std::format("[R{}]", baseIndex);
        }
        else
        {
            address = std::format("[R{}, {}]{}", baseIndex, expression, instruction_.flags.W ? "!" : "");
        }
    }
    else
    {
        // Post-indexed
        if (expression == "")
        {
            address = std::format("[R{}]", baseIndex);
        }
        else
        {
            address = std::format("[R{}], {}", baseIndex, expression);
        }
    }

    mnemonic_ = std::format("{:08X} -> {} R{}, {}", instruction_.word, op, srcDestIndex, address);
}

void SingleDataSwap::SetMnemonic()
{
    std::string cond = Logging::ConditionMnemonic(instruction_.flags.Cond);
    std::string b = instruction_.flags.B ? "B" : "";
    std::string op = "SWP" + cond + b;

    uint8_t rd = instruction_.flags.Rd;
    uint8_t rm = instruction_.flags.Rm;
    uint8_t rn = instruction_.flags.Rn;
    std::string regString = std::format("R{}, R{}, [R{}]", rd, rm, rn);

    mnemonic_ = std::format("{:08X} -> {} {}", instruction_.word, op, regString);
}

void Multiply::SetMnemonic()
{
    std::string cond = Logging::ConditionMnemonic(instruction_.flags.Cond);
    std::string s = instruction_.flags.S ? "S" : "";
    uint8_t rd = instruction_.flags.Rd;
    uint8_t rm = instruction_.flags.Rm;
    uint8_t rs = instruction_.flags.Rs;
    uint8_t rn = instruction_.flags.Rn;

    if (instruction_.flags.A)
    {
        mnemonic_ = std::format("{:08X} -> MLA{}{} R{}, R{}, R{}, R{}", instruction_.word, cond, s, rd, rm, rs, rn);
    }
    else
    {
        mnemonic_ = std::format("{:08X} -> MUL{}{} R{}, R{}, R{}", instruction_.word, cond, s, rd, rm, rs);
    }
}

void MultiplyLong::SetMnemonic()
{
    std::string cond = Logging::ConditionMnemonic(instruction_.flags.Cond);
    std::string u = instruction_.flags.U ? "S" : "U";
    std::string s = instruction_.flags.S ? "S" : "";
    uint8_t rdHi = instruction_.flags.RdHi;
    uint8_t rdLo = instruction_.flags.RdLo;
    uint8_t rs = instruction_.flags.Rs;
    uint8_t rm = instruction_.flags.Rm;
    std::string regString = std::format("R{}, R{}, R{}, R{}", rdLo, rdHi, rm, rs);

    if (instruction_.flags.A)
    {
        mnemonic_ = std::format("{:08X} -> {}MLAL{}{} {}", instruction_.word, u, cond, s, regString);
    }
    else
    {
        mnemonic_ = std::format("{:08X} -> {}MULL{}{} {}", instruction_.word, u, cond, s, regString);
    }
}

void HalfwordDataTransferRegisterOffset::SetMnemonic(uint32_t offset)
{
    uint8_t offsetRegIndex = instruction_.flags.Rm;
    std::string offsetExpression = (offset == 0 ? "" : std::format("R{}", offsetRegIndex));
    std::string mnemonic = HalfwordDataTransferHelper(instruction_.flags.L,
                                                      instruction_.flags.Cond,
                                                      instruction_.flags.Rd,
                                                      instruction_.flags.Rn,
                                                      instruction_.flags.S,
                                                      instruction_.flags.H,
                                                      instruction_.flags.U,
                                                      instruction_.flags.P,
                                                      instruction_.flags.W,
                                                      offsetExpression);

    mnemonic_ = std::format("{:08X} -> {}", instruction_.word, mnemonic);
}

void HalfwordDataTransferImmediateOffset::SetMnemonic(uint8_t offset)
{
    std::string offsetExpression = (offset == 0 ? "" : std::format("#{}", offset));
    std::string mnemonic = HalfwordDataTransferHelper(instruction_.flags.L,
                                                      instruction_.flags.Cond,
                                                      instruction_.flags.Rd,
                                                      instruction_.flags.Rn,
                                                      instruction_.flags.S,
                                                      instruction_.flags.H,
                                                      instruction_.flags.U,
                                                      instruction_.flags.P,
                                                      instruction_.flags.W,
                                                      offsetExpression);

    mnemonic_ = std::format("{:08X} -> {}", instruction_.word, mnemonic);
}

void PSRTransferMRS::SetMnemonic()
{
    std::string cond = Logging::ConditionMnemonic(instruction_.flags.Cond);
    std::string psr = instruction_.flags.Ps ? "SPSR" : "CPSR";
    uint8_t rd = instruction_.flags.Rd;
    mnemonic_ = std::format("{:08X} -> MRS{} R{}, {}", instruction_.word, cond, rd, psr);
}

void PSRTransferMSR::SetMnemonic()
{
    std::string cond = Logging::ConditionMnemonic(instruction_.commonFlags.Cond);

    std::stringstream fields;
    fields << "_";
    fields << (instruction_.commonFlags.Flags ? "f" : "");
    fields << (instruction_.commonFlags.Status ? "s" : "");
    fields << (instruction_.commonFlags.Extension ? "x" : "");
    fields << (instruction_.commonFlags.Control ? "c" : "");
    std::string fieldsStr = fields.str() == "_fsxc" ? "_all" : fields.str();

    std::string psr = instruction_.commonFlags.Pd ? "SPSR" : "CPSR";
    psr = psr + fieldsStr;
    std::string expression;

    if (instruction_.commonFlags.I)
    {
        uint32_t imm = instruction_.immFlag.Imm;
        imm = std::rotr(imm, instruction_.immFlag.Rotate * 2);
        expression = std::format("{}, #{:08X}", psr, imm);
    }
    else
    {
        uint8_t rm = instruction_.regFlags.Rm;
        expression = std::format("{}, R{}", psr, rm);
    }

    mnemonic_ = std::format("{:08X} -> MSR{} {}", instruction_.word, cond, expression);
}

void DataProcessing::SetMnemonic(uint32_t operand2)
{
    std::string op;
    std::string cond = Logging::ConditionMnemonic(instruction_.flags.Cond);
    std::string s = instruction_.flags.S ? "S" : "";

    if ((instruction_.flags.OpCode >= 8) && (instruction_.flags.OpCode <= 11))
    {
        s = "";
    }

    switch (instruction_.flags.OpCode)
    {
        case 0b0000:
            op = "AND";
            break;
        case 0b0001:
            op = "EOR";
            break;
        case 0b0010:
            op = "SUB";
            break;
        case 0b0011:
            op = "RSB";
            break;
        case 0b0100:
            op = "ADD";
            break;
        case 0b0101:
            op = "ADC";
            break;
        case 0b0110:
            op = "SBC";
            break;
        case 0b0111:
            op = "RSC";
            break;
        case 0b1000:
            op = "TST";
            break;
        case 0b1001:
            op = "TEQ";
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
            op = "MOV";
            break;
        case 0b1110:
            op = "BIC";
            break;
        case 0b1111:
            op = "MVN";
            break;
    }

    std::string regInfo = "";
    uint8_t const destIndex = instruction_.flags.Rd;
    uint8_t const operand1Index = instruction_.flags.Rn;

    std::string operand2Str;

    if (instruction_.flags.I)
    {
        // Rotated immediate operand
        operand2Str = std::format("#{}", operand2);
    }
    else
    {
        std::string shiftType;
        bool shiftByReg = instruction_.flags.Operand2 & 0x10;
        bool isRRX = !shiftByReg && (instruction_.shiftRegByImm.ShiftAmount == 0) && (instruction_.shiftRegByReg.ShiftType == 3);

        uint8_t Rm = instruction_.shiftRegByReg.Rm;
        uint8_t Rs = instruction_.shiftRegByReg.Rs;
        uint8_t shiftAmount = instruction_.shiftRegByImm.ShiftAmount;

        switch (instruction_.shiftRegByReg.ShiftType)
        {
            case 0b00:
                shiftType = "LSL";
                break;
            case 0b01:
                shiftType = "LSR";
                shiftAmount = (shiftAmount == 0) ? 32 : shiftAmount;
                break;
            case 0b10:
                shiftType = "ASR";
                shiftAmount = (shiftAmount == 0) ? 32 : shiftAmount;
                break;
            case 0b11:
                shiftType = isRRX ? "RRX" : "ROR";
                break;
        }

        if (shiftByReg)
        {
            // Shift reg by reg
            operand2Str = std::format("R{}, {} R{}", Rm, shiftType, Rs);
        }
        else
        {
            // Shift reg by imm
            if (isRRX)
            {
                operand2Str = std::format("R{}, {}", Rm, shiftType);
            }
            else if ((shiftType == "LSL") && (instruction_.shiftRegByImm.ShiftAmount == 0))
            {
                operand2Str = std::format("R{}", Rm);
            }
            else
            {
                operand2Str = std::format("R{}, {} #{}", Rm, shiftType, shiftAmount);
            }
        }
    }

    switch (instruction_.flags.OpCode)
    {
        case 0b1101:  // MOV
        case 0b1111:  // MVN
            regInfo = std::format("R{}, {}", destIndex, operand2Str);
            break;
        case 0b1010:  // CMP
        case 0b1011:  // CMN
        case 0b1001:  // TEQ
        case 0b1000:  // TST
            regInfo = std::format("R{}, {}", operand1Index, operand2Str);
            break;
        case 0b0000:  // AND
        case 0b0001:  // EOR
        case 0b0010:  // SUB
        case 0b0011:  // RSB
        case 0b0100:  // ADD
        case 0b0101:  // ADC
        case 0b0110:  // SBC
        case 0b0111:  // RSC
        case 0b1100:  // ORR
        case 0b1110:  // BIC
            regInfo = std::format("R{}, R{}, {}", destIndex, operand1Index, operand2Str);
            break;
    }

    mnemonic_ = std::format("{:08X} -> {}{}{} {}", instruction_.word, op, cond, s, regInfo);
}
}
