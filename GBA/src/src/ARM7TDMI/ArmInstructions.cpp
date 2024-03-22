#include <ARM7TDMI/ArmInstructions.hpp>
#include <ARM7TDMI/ARM7TDMI.hpp>
#include <cstdint>
#include <memory>
#include <stdexcept>
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
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: ARM_Branch");
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
