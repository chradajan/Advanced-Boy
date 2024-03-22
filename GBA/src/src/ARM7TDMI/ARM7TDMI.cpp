#include <ARM7TDMI/ARM7TDMI.hpp>
#include <ARM7TDMI/ArmInstructions.hpp>
#include <ARM7TDMI/CpuTypes.hpp>
#include <ARM7TDMI/ThumbInstructions.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>

namespace CPU
{
ARM7TDMI::ARM7TDMI(std::function<uint32_t(uint32_t, uint8_t)> ReadMemory,
                   std::function<void(uint32_t, uint32_t, uint8_t)> WriteMemory) :
    rawFetchedInstruction_(MAX_U32),
    decodedInstruction_(nullptr),
    ReadMemory(ReadMemory),
    WriteMemory(WriteMemory)
{
}

void ARM7TDMI::Clock()
{
    bool const isArmState = registers_.GetOperatingState() == OperatingState::ARM;
    uint8_t const accessSize = isArmState ? 4 : 2;

    // Execute
    if (decodedInstruction_)
    {
        decodedInstruction_->Execute(*this);
    }

    // Decode
    if (rawFetchedInstruction_ != MAX_U32)
    {
        decodedInstruction_.reset();
        
        if (isArmState)
        {
            decodedInstruction_ = ARM::DecodeInstruction(rawFetchedInstruction_);
        }
        else
        {
            decodedInstruction_ = THUMB::DecodeInstruction(rawFetchedInstruction_);
        }
    }

    // Fetch
    rawFetchedInstruction_ = ReadMemory(registers_.GetPC(), accessSize);
    registers_.AdvancePC();


    // bool const isArmState = registers_.GetOperatingState() == OperatingState::ARM;
    // uint8_t const accessSize = isArmState ? 4 : 2;
    // uint32_t const rawInstruction = ReadMemory(registers_.GetPC(), accessSize);

    // std::unique_ptr<Instruction> decodedInstruction;

    // if (isArmState)
    // {
    //     decodedInstruction = ARM::DecodeInstruction(rawInstruction);
    // }
    // else
    // {
    //     decodedInstruction = THUMB::DecodeInstruction(rawInstruction);
    // }

    // registers_.AdvancePC();
    // decodedInstruction->Execute(*this);
}

bool ARM7TDMI::ArmConditionMet(uint8_t condition)
{
    switch (condition)
    {
        case 0:  // EQ
            return registers_.IsZero();
        case 1:  // NE
            return !registers_.IsZero();
        case 2:  // CS
            return registers_.IsCarry();
        case 3:  // CC
            return !registers_.IsCarry();
        case 4:  // MI
            return registers_.IsNegative();
        case 5:  // PL
            return !registers_.IsNegative();
        case 6:  // VS
            return registers_.IsOverflow();
        case 7:  // VC
            return !registers_.IsOverflow();
        case 8:  // HI
            return registers_.IsCarry() && !registers_.IsZero();
        case 9:  // LS
            return !registers_.IsCarry() && registers_.IsZero();
        case 10: // GE
            return registers_.IsNegative() == registers_.IsOverflow();
        case 11: // LT
            return registers_.IsNegative() != registers_.IsOverflow();
        case 12: // GT
            return !registers_.IsZero() && (registers_.IsNegative() == registers_.IsOverflow());
        case 13: // LE
            return registers_.IsZero() || (registers_.IsNegative() != registers_.IsOverflow());
        case 14: // AL
            return true;
    }

    throw std::runtime_error("Illegal ARM condition");
}
}
