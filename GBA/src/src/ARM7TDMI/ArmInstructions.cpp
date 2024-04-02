#include <ARM7TDMI/ArmInstructions.hpp>
#include <ARM7TDMI/ARM7TDMI.hpp>
#include <Config.hpp>
#include <cstdint>
#include <format>
#include <memory>
#include <sstream>
#include <string>
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

int BranchAndExchange::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: ARM_BranchAndExchange");
}

int BlockDataTransfer::Execute(ARM7TDMI& cpu)
{
    int cycles = 1;

    if constexpr (Config::LOGGING_ENABLED)
    {
        SetMnemonic();
    }

    if (!cpu.ArmConditionMet(instruction_.flags.Cond))
    {
        return cycles;
    }

    uint8_t regIndex = 0;
    uint16_t regList = instruction_.flags.RegisterList;
    uint32_t addr = cpu.registers_.ReadRegister(instruction_.flags.Rn) & 0xFFFF'FFFC;
    bool const r15InList = instruction_.flags.RegisterList & 0x8000;
    bool const preIndexOffset = instruction_.flags.P;
    bool const postIndexOffset = !preIndexOffset;
    int8_t const offset = instruction_.flags.U ? 4 : -4;

    OperatingMode mode = cpu.registers_.GetOperatingMode();

    if (instruction_.flags.S)
    {
        if (r15InList)
        {
            if (!instruction_.flags.L)
            {
                mode = OperatingMode::User;
            }
        }
        else
        {
            mode = OperatingMode::User;
        }
    }

    while (regList != 0)
    {
        if (regList & 0x01)
        {
            if (preIndexOffset)
            {
                addr += offset;
            }

            if (instruction_.flags.L)
            {
                auto [readValue, readCycles] = cpu.ReadMemory(addr, 4);
                cycles += readCycles;
                cpu.registers_.WriteRegister(regIndex, readValue, mode);
            }
            else
            {
                uint32_t regValue = cpu.registers_.ReadRegister(regIndex, mode);
                int writeCycles = cpu.WriteMemory(addr, regValue, 4);
                cycles += writeCycles;
            }

            if (postIndexOffset)
            {
                addr += offset;
            }
        }

        ++regIndex;
        regList >>= 1;
    }

    if (instruction_.flags.W)
    {
        cpu.registers_.WriteRegister(instruction_.flags.Rn, addr);
    }

    if (instruction_.flags.L && r15InList)
    {
        cpu.flushPipeline_ = true;

        if (instruction_.flags.S)
        {
            cpu.registers_.LoadSPSR();
        }
    }

    return cycles;
}

int Branch::Execute(ARM7TDMI& cpu)
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
        SetMnemonic(newPC);
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
        cpu.flushPipeline_ = true;
    }

    return 1;
}

int SoftwareInterrupt::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: ARM_SoftwareInterrupt");
}

int Undefined::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: ARM_Undefined");
}

int SingleDataTransfer::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: ARM_SingleDataTransfer");
}

int SingleDataSwap::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: ARM_SingleDataSwap");
}

int Multiply::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: ARM_Multiply");
}

int MultiplyLong::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: ARM_MultiplyLong");
}

int HalfwordDataTransferRegisterOffset::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: ARM_HalfwordDataTransferRegisterOffset");
}

int HalfwordDataTransferImmediateOffset::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: ARM_HalfwordDataTransferImmediateOffset");
}

int PSRTransferMRS::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: ARM_PSRTransferMRS");
}

int PSRTransferMSR::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: ARM_PSRTransferMSR");
}

int DataProcessing::Execute(ARM7TDMI& cpu)
{
    (void)cpu;
    throw std::runtime_error("Unimplemented Instruction: ARM_DataProcessing");
}
}
