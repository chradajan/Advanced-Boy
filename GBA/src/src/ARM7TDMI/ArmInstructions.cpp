#include <ARM7TDMI/ArmInstructions.hpp>
#include <ARM7TDMI/ARM7TDMI.hpp>
#include <ARM7TDMI/CpuTypes.hpp>
#include <Config.hpp>
#include <bit>
#include <cstdint>
#include <format>
#include <memory>
#include <sstream>
#include <string>
#include <stdexcept>

namespace
{
/// @brief Helper function to carry out subtraction operations for Data Processing.
/// @param operand1 Left hand operand.
/// @param operand2 Right hand operand.
/// @param overflowOut Result of V flag after subtraction.
/// @param carryOut Result of C flag after subtraction.
/// @param useCarry Whether to include carry bit in subtraction.
/// @param carry Value of carry bit.
/// @return Result of subtraction operation.
uint32_t DataProcessingSubtraction(uint32_t operand1,
                                   uint32_t operand2,
                                   bool& overflowOut,
                                   bool& carryOut,
                                   bool useCarry=false,
                                   bool carry=false)
{
    uint64_t result64;
    uint8_t carryVal = carry ? 1 : 0;

    if (useCarry)
    {
        result64 = operand1 - operand2 + carryVal - 1;
    }
    else
    {
        result64 = operand1 - operand2;
    }

    uint32_t result32 = result64 & CPU::MAX_U32;
    overflowOut = ((operand1 & 0x8000'0000) != (operand2 & 0x8000'0000)) && ((operand2 & 0x8000'0000) == (result32 & 0x8000'0000));
    carryOut = result64 > CPU::MAX_U32;

    return result32;
}

/// @brief Helper function to carry out addition operations for Data Processing.
/// @param operand1 Left hand operand.
/// @param operand2 Right hand operand.
/// @param overflowOut Result of V flag after addition.
/// @param carryOut Result of C flag after addition.
/// @param useCarry Whether to include carry bit in addition.
/// @param carry Value of carry bit.
/// @return Result of addition operation.
uint32_t DataProcessingAddition(uint32_t operand1,
                                uint32_t operand2,
                                bool& overflowOut,
                                bool& carryOut,
                                bool useCarry=false,
                                bool carry=false)
{
    uint64_t result64;
    uint8_t carryVal = carry ? 1 : 0;

    if (useCarry)
    {
        result64 = operand1 + operand2 + carryVal;
    }
    else
    {
        result64 = operand1 + operand2;
    }

    uint32_t result32 = result64 & CPU::MAX_U32;
    overflowOut = ((operand1 & 0x8000'0000) == (operand2 & 0x8000'0000)) && ((operand1 & 0x8000'0000) != (result32 & 0x8000'0000));
    carryOut = result64 > CPU::MAX_U32;

    return result32;
}
}

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

    uint16_t regList = instruction_.flags.RegisterList;
    bool r15InList = instruction_.flags.RegisterList & 0x8000;
    OperatingMode mode = cpu.registers_.GetOperatingMode();

    if (!regList)
    {
        // If regList is empty, R15 is still stored/loaded
        regList = 0x8000;
    }

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

    int regListSize = std::popcount(regList);
    uint32_t baseAddr = cpu.registers_.ReadRegister(instruction_.flags.Rn) & 0xFFFF'FFFC;
    uint32_t minAddr;
    uint32_t wbAddr;
    bool preIndexOffset = instruction_.flags.P;

    if (instruction_.flags.U)
    {
        minAddr = preIndexOffset ? (baseAddr + 4) : baseAddr;

        if (preIndexOffset)
        {
            wbAddr = minAddr + (4 * (regListSize - 1));
        }
        else
        {
            wbAddr = minAddr + (4 * regListSize);
        }
    }
    else
    {
        if (preIndexOffset)
        {
            minAddr = baseAddr - (4 * regListSize);
            wbAddr = minAddr;
        }
        else
        {
            minAddr = baseAddr - (4 * (regListSize - 1));
            wbAddr = minAddr - 4;
        }
    }

    uint8_t regIndex = 0;
    uint32_t addr = minAddr;

    while (regList != 0)
    {
        if (regList & 0x01)
        {
            if (instruction_.flags.L)
            {
                auto [readValue, readCycles] = cpu.ReadMemory(addr, 4);
                cycles += readCycles;
                cpu.registers_.WriteRegister(regIndex, readValue, mode);
            }
            else
            {
                uint32_t regValue = cpu.registers_.ReadRegister(regIndex, mode);

                if (regIndex == 15)
                {
                    regValue += 4;
                }

                int writeCycles = cpu.WriteMemory(addr, regValue, 4);
                cycles += writeCycles;
            }

            addr += 4;
        }

        ++regIndex;
        regList >>= 1;
    }

    if (instruction_.flags.W)
    {
        cpu.registers_.WriteRegister(instruction_.flags.Rn, wbAddr);
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
    int cycles = 1;

    uint32_t unsignedOffset = (instruction_.flags.Rm);
    int16_t signedOffset = instruction_.flags.U ? unsignedOffset : -unsignedOffset;
    bool preIndex = instruction_.flags.P;
    bool postIndex = !preIndex;
    uint32_t addr = cpu.registers_.ReadRegister(instruction_.flags.Rn);

    if constexpr (Config::LOGGING_ENABLED)
    {
        SetMnemonic(unsignedOffset);
    }

    if (preIndex)
    {
        addr += signedOffset;
    }

    if (instruction_.flags.L)  // Load
    {
        cpu.flushPipeline_ = (instruction_.flags.Rd == 15);

        if (instruction_.flags.S)
        {
            uint32_t signExtendedWord;

            if (instruction_.flags.H)
            {
                // S = 1, H = 1
                auto [halfWord, readCycles] = cpu.ReadMemory(addr, 2);
                signExtendedWord = halfWord;

                if (halfWord & 0x8000)
                {
                    signExtendedWord |= 0xFFFF'0000;
                }

                cpu.registers_.WriteRegister(instruction_.flags.Rd, signExtendedWord);
                cycles += readCycles;
            }
            else
            {
                // S = 1, H = 0
                auto [byte, readCycles] = cpu.ReadMemory(addr, 1);
                signExtendedWord = byte;

                if (byte & 0x80)
                {
                    signExtendedWord |= 0xFFFF'FF00;
                }

                cpu.registers_.WriteRegister(instruction_.flags.Rd, signExtendedWord);
                cycles += readCycles;
            }
        }
        else
        {
            // S = 0, H = 1
            auto [halfWord, readCycles] = cpu.ReadMemory(addr, 2);
            cpu.registers_.WriteRegister(instruction_.flags.Rd, halfWord);
            cycles += readCycles;
        }
    }
    else  // Store
    {
        // S = 0, H = 1
        uint8_t srcIndex = instruction_.flags.Rd;
        uint16_t halfWord = cpu.registers_.ReadRegister(srcIndex);

        if (srcIndex == 15)
        {
            halfWord += 4;
        }

        cycles += cpu.WriteMemory(addr, halfWord, 2);
    }

    if (postIndex)
    {
        addr += signedOffset;
    }

    if (instruction_.flags.W)
    {
        cpu.registers_.WriteRegister(instruction_.flags.Rn, addr);
    }

    return cycles;
}

int HalfwordDataTransferImmediateOffset::Execute(ARM7TDMI& cpu)
{
    int cycles = 1;

    uint8_t unsignedOffset = (instruction_.flags.Offset1 << 4) | instruction_.flags.Offset;
    int16_t signedOffset = instruction_.flags.U ? unsignedOffset : -unsignedOffset;
    bool preIndex = instruction_.flags.P;
    bool postIndex = !preIndex;
    uint32_t addr = cpu.registers_.ReadRegister(instruction_.flags.Rn);

    if constexpr (Config::LOGGING_ENABLED)
    {
        SetMnemonic(unsignedOffset);
    }

    if (preIndex)
    {
        addr += signedOffset;
    }

    if (instruction_.flags.L)  // Load
    {
        cpu.flushPipeline_ = (instruction_.flags.Rd == 15);

        if (instruction_.flags.S)
        {
            uint32_t signExtendedWord;

            if (instruction_.flags.H)
            {
                // S = 1, H = 1
                auto [halfWord, readCycles] = cpu.ReadMemory(addr, 2);
                signExtendedWord = halfWord;

                if (halfWord & 0x8000)
                {
                    signExtendedWord |= 0xFFFF'0000;
                }

                cpu.registers_.WriteRegister(instruction_.flags.Rd, signExtendedWord);
                cycles += readCycles;
            }
            else
            {
                // S = 1, H = 0
                auto [byte, readCycles] = cpu.ReadMemory(addr, 1);
                signExtendedWord = byte;

                if (byte & 0x80)
                {
                    signExtendedWord |= 0xFFFF'FF00;
                }

                cpu.registers_.WriteRegister(instruction_.flags.Rd, signExtendedWord);
                cycles += readCycles;
            }
        }
        else
        {
            // S = 0, H = 1
            auto [halfWord, readCycles] = cpu.ReadMemory(addr, 2);
            cpu.registers_.WriteRegister(instruction_.flags.Rd, halfWord);
            cycles += readCycles;
        }
    }
    else  // Store
    {
        // S = 0, H = 1
        uint8_t srcIndex = instruction_.flags.Rd;
        uint16_t halfWord = cpu.registers_.ReadRegister(srcIndex);

        if (srcIndex == 15)
        {
            halfWord += 4;
        }

        cycles += cpu.WriteMemory(addr, halfWord, 2);
    }

    if (postIndex)
    {
        addr += signedOffset;
    }

    if (instruction_.flags.W)
    {
        cpu.registers_.WriteRegister(instruction_.flags.Rn, addr);
    }

    return cycles;
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
    int cycles = 1;

    uint32_t operand1 = cpu.registers_.ReadRegister(instruction_.flags.Rn);
    uint32_t operand2;
    uint8_t destIndex = instruction_.flags.Rd;

    bool negativeOut = cpu.registers_.IsNegative();
    bool zeroOut = cpu.registers_.IsZero();
    bool carryOut = cpu.registers_.IsCarry();
    bool overflowOut = cpu.registers_.IsOverflow();

    if (instruction_.flags.I)
    {
        // Operand 2 is an immediate value
        operand2 = instruction_.rotatedImmediate.Imm;
        uint8_t rotateAmount = instruction_.rotatedImmediate.RotateAmount << 1;
        operand2 = std::rotr(operand2, rotateAmount);
    }
    else
    {
        // Operand 2 is a register
        operand2 = cpu.registers_.ReadRegister(instruction_.shiftRegByReg.Rm);

        ++cycles;

        // If R15 is used as an operand and a register specified shift amount is used, PC will be 12 bytes ahead.
        if (instruction_.flags.Rn == 15)
        {
            operand1 += 4;
        }

        if (instruction_.shiftRegByReg.Rm == 15)
        {
            operand2 += 4;
        }

        bool shiftByReg = (instruction_.flags.Operand2 & 0x10);
        uint8_t shiftAmount =
            shiftByReg ? (cpu.registers_.ReadRegister(instruction_.shiftRegByReg.Rs) & 0xFF) :
            instruction_.shiftRegByImm.ShiftAmount;

        switch (instruction_.shiftRegByReg.ShiftType)
        {
            case 0b00:  // LSL
            {
                if (shiftAmount == 0)
                {
                    carryOut = cpu.registers_.IsCarry();
                }
                else if (shiftAmount > 32)
                {
                    carryOut = false;
                    operand2 = 0;
                }
                else
                {
                    carryOut = (operand2 & (0x8000'0000 >> (shiftAmount - 1)));
                    operand2 <<= shiftAmount;
                }
                break;
            }
            case 0b01:  // LSR
            {
                if (shiftAmount == 0)
                {
                    if (shiftByReg)
                    {
                        carryOut = cpu.registers_.IsCarry();
                    }
                    else
                    {
                        // LSR #0 -> LSR #32
                        carryOut = (operand2 & 0x8000'0000);
                        operand2 = 0;
                    }
                }
                else if (shiftAmount > 32)
                {
                    carryOut = false;
                    operand2 = 0;
                }
                else
                {
                    carryOut = (operand2 & (0x01 << (shiftAmount - 1)));
                    operand2 >>= shiftAmount;
                }
                break;
            }
            case 0b10:  // ASR
            {
                bool msbSet = operand2 & 0x8000'0000;

                if (shiftAmount == 0)
                {
                    if (shiftByReg)
                    {
                        carryOut = cpu.registers_.IsCarry();
                    }
                    else
                    {
                        // ASR #0 -> ASR #32
                        carryOut = msbSet;
                        operand2 = msbSet ? 0xFFFF'FFFF : 0;
                    }
                }
                else if (shiftAmount > 32)
                {
                    carryOut = msbSet;
                    operand2 = msbSet ? 0xFFFF'FFFF : 0;
                }
                else
                {
                    carryOut = (operand2 & (0x01 << (shiftAmount - 1)));

                    for (int i = 0; i < shiftAmount; ++i)
                    {
                        operand2 >>= 1;
                        operand2 |= msbSet ? 0x8000'0000 : 0;
                    }
                }
                break;
            }
            case 0b11:  // ROR, RRX
            {
                if (shiftAmount > 32)
                {
                    shiftAmount %= 32;
                }

                if (shiftAmount == 0)
                {
                    if (shiftByReg)
                    {
                        carryOut = cpu.registers_.IsCarry();
                    }
                    else
                    {
                        // ROR #0 -> RRX
                        carryOut = operand2 & 0x01;
                        operand2 >>= 1;
                        operand2 |= cpu.registers_.IsCarry() ? 0x8000'0000 : 0;
                    }
                }
                else
                {
                    carryOut = (operand2 & (0x01 << (shiftAmount - 1)));
                    operand2 = std::rotr(operand2, shiftAmount);
                }
                break;
            }
        }
    }

    if constexpr (Config::LOGGING_ENABLED)
    {
        SetMnemonic(operand2);
    }

    if (!cpu.ArmConditionMet(instruction_.flags.Cond))
    {
        return cycles;
    }

    uint32_t result = 0;
    bool writeResult = true;
    bool updateFlags = instruction_.flags.S;

    switch (instruction_.flags.OpCode)
    {
        case 0b0000:  // AND
            result = operand1 & operand2;
            break;
        case 0b0001:  // EOR
            result = operand1 & operand2;
            break;
        case 0b0010:  // SUB
            result = DataProcessingSubtraction(operand1, operand2, overflowOut, carryOut);
            break;
        case 0b0011:  // RSB
            result = DataProcessingSubtraction(operand2, operand1, overflowOut, carryOut);
            break;
        case 0b0100:  // ADD
            result = DataProcessingAddition(operand1, operand2, overflowOut, carryOut);
            break;
        case 0b0101:  // ADC
            result = DataProcessingAddition(operand1, operand2, overflowOut, carryOut, true, cpu.registers_.IsCarry());
            break;
        case 0b0110:  // SBC
            result = DataProcessingSubtraction(operand1, operand2, overflowOut, carryOut, true, cpu.registers_.IsCarry());
            break;
        case 0b0111:  // RSC
            result = DataProcessingSubtraction(operand2, operand1, overflowOut, carryOut, true, cpu.registers_.IsCarry());
            break;
        case 0b1000:  // TST
            result = operand1 & operand2;
            writeResult = false;
            break;
        case 0b1001:  // TEQ
            result = operand1 & operand2;
            writeResult = false;
            break;
        case 0b1010:  // CMP
            result = DataProcessingSubtraction(operand1, operand2, overflowOut, carryOut);
            writeResult = false;
            break;
        case 0b1011:  // CMN
            result = DataProcessingAddition(operand1, operand2, overflowOut, carryOut);
            writeResult = false;
            break;
        case 0b1100:  // ORR
            result = operand1 | operand2;
            break;
        case 0b1101:  // MOV
            result = operand2;
            break;
        case 0b1110:  // BIC
            result = operand1 & ~operand2;
            break;
        case 0b1111:  // MVN
            result = ~operand2;
            break;
    }

    negativeOut = (result & 0x8000'0000);
    zeroOut = (result == 0);

    if (writeResult)
    {
        if (destIndex == 15)
        {
            ++cycles;
            cpu.flushPipeline_ = true;

            if (updateFlags)
            {
                updateFlags = false;
                cpu.registers_.LoadSPSR();
            }
        }

        cpu.registers_.WriteRegister(destIndex, result);
    }

    if (updateFlags)
    {
        cpu.registers_.SetNegative(negativeOut);
        cpu.registers_.SetZero(zeroOut);
        cpu.registers_.SetCarry(carryOut);
        cpu.registers_.SetOverflow(overflowOut);
    }

    return cycles;
}
}
