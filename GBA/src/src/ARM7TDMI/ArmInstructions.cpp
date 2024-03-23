#include <ARM7TDMI/ArmInstructions.hpp>
#include <ARM7TDMI/ARM7TDMI.hpp>
#include <Config.hpp>
#include <cstdint>
#include <format>
#include <memory>
#include <sstream>
#include <string>
#include <stdexcept>
#include <iomanip>
namespace CPU::ARM
{
std::unique_ptr<ArmInstruction> DecodeInstruction(uint32_t const instruction)
{
    if (BranchAndExchange::IsInstanceOf(instruction))
    {
        return std::make_unique<BranchAndExchange>(instruction);
    }
    else if (BlockDataTransfer::IsInstanceOf(instruction))
    {
        return std::make_unique<BlockDataTransfer>(instruction);
    }
    else if (Branch::IsInstanceOf(instruction))
    {
        return std::make_unique<Branch>(instruction);
    }
    else if (SoftwareInterrupt::IsInstanceOf(instruction))
    {
        return std::make_unique<SoftwareInterrupt>(instruction);
    }
    else if (Undefined::IsInstanceOf(instruction))
    {
        return std::make_unique<Undefined>(instruction);
    }
    else if (SingleDataTransfer::IsInstanceOf(instruction))
    {
        return std::make_unique<SingleDataTransfer>(instruction);
    }
    else if (SingleDataSwap::IsInstanceOf(instruction))
    {
        return std::make_unique<SingleDataSwap>(instruction);
    }
    else if (Multiply::IsInstanceOf(instruction))
    {
        return std::make_unique<Multiply>(instruction);
    }
    else if (MultiplyLong::IsInstanceOf(instruction))
    {
        return std::make_unique<MultiplyLong>(instruction);
    }
    else if (HalfwordDataTransferRegisterOffset::IsInstanceOf(instruction))
    {
        return std::make_unique<HalfwordDataTransferRegisterOffset>(instruction);
    }
    else if (HalfwordDataTransferImmediateOffset::IsInstanceOf(instruction))
    {
        return std::make_unique<HalfwordDataTransferImmediateOffset>(instruction);
    }
    else if (PSRTransferMRS::IsInstanceOf(instruction))
    {
        return std::make_unique<PSRTransferMRS>(instruction);
    }
    else if (PSRTransferMSR::IsInstanceOf(instruction))
    {
        return std::make_unique<PSRTransferMSR>(instruction);
    }
    else if (DataProcessing::IsInstanceOf(instruction))
    {
        return std::make_unique<DataProcessing>(instruction);
    }

    return nullptr;
}

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

void BranchAndExchange::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: ARM_BranchAndExchange");
}

void BlockDataTransfer::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: ARM_BlockDataTransfer");
}

void Branch::Execute(ARM7TDMI& cpu)
{
    // Branch instructions contain a signed 2's complement 24 bit offset. This is shifted left
    // two bits, sign extended to 32 bits, and added to the PC. The instruction can therefore
    // specify a branch of +/- 32Mbytes. The branch offset must take account of the prefetch
    // operation, which causes the PC to be 2 words (8 bytes) ahead of the current
    // instruction.
    int32_t signedOffset = instruction_.flags.Offset << 2;

    if (signedOffset & 0x0200'0000)
    {
        signedOffset |= 0xFC00'0000;
    }

    uint32_t newPC = cpu.registers_.GetPC() + signedOffset;

    if constexpr (Config::LOGGING_ENABLED)
    {
        std::string op = instruction_.flags.L ? "BL" : "B";
        std::string cond = ConditionMnemonic(instruction_.flags.Cond);
        mnemonic_ = std::format("{}{} {:08X}", op, cond, newPC);
    }

    if (cpu.ArmConditionMet(instruction_.flags.Cond))
    {

        // Branch with Link (BL) writes the old PC into the link register (R14) of the current bank.
        // The PC value written into R14 is adjusted to allow for the prefetch, and contains the
        // address of the instruction following the branch and link instruction. Note that the CPSR
        // is not saved with the PC and R14[1:0] are always cleared.
        if (instruction_.flags.L)
        {
            cpu.registers_.WriteRegister(14, (cpu.registers_.GetPC() - 4) & 0xFFFF'FFFC);
        }

        cpu.registers_.SetPC(newPC);
        cpu.branchExecuted_ = true;
    }
}

void SoftwareInterrupt::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: ARM_SoftwareInterrupt");
}

void Undefined::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: ARM_Undefined");
}

void SingleDataTransfer::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: ARM_SingleDataTransfer");
}

void SingleDataSwap::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: ARM_SingleDataSwap");
}

void Multiply::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: ARM_Multiply");
}

void MultiplyLong::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: ARM_MultiplyLong");
}

void HalfwordDataTransferRegisterOffset::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: ARM_HalfwordDataTransferRegisterOffset");
}

void HalfwordDataTransferImmediateOffset::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: ARM_HalfwordDataTransferImmediateOffset");
}

void PSRTransferMRS::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: ARM_PSRTransferMRS");
}

void PSRTransferMSR::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: ARM_PSRTransferMSR");
}

void DataProcessing::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: ARM_DataProcessing");
}
}
