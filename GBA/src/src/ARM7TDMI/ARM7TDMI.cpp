#include <ARM7TDMI/ARM7TDMI.hpp>
#include <ARM7TDMI/ArmInstructions.hpp>
#include <ARM7TDMI/CpuTypes.hpp>
#include <ARM7TDMI/ThumbInstructions.hpp>
#include <Config.hpp>
#include <Logging/Logging.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <queue>

namespace CPU
{
ARM7TDMI::ARM7TDMI(std::function<std::pair<uint32_t, int>(uint32_t, uint8_t)> ReadMemory,
                   std::function<int(uint32_t, uint32_t, uint8_t)> WriteMemory) :
    decodedInstruction_(nullptr),
    flushPipeline_(false),
    ReadMemory(ReadMemory),
    WriteMemory(WriteMemory)
{
}

void ARM7TDMI::Clock()
{
    bool const isArmState = registers_.GetOperatingState() == OperatingState::ARM;
    uint8_t const accessSize = isArmState ? 4 : 2;
    uint32_t undecodedInstruction = MAX_U32;

    // Execute
    if (decodedInstruction_)
    {
        uint32_t loggedPC;
        std::string regString;

        if constexpr (Config::LOGGING_ENABLED)
        {
            loggedPC = isArmState ? registers_.GetPC() - 8 : registers_.GetPC() - 4;
            regString = registers_.GetRegistersString();
        }

        decodedInstruction_->Execute(*this);

        if constexpr (Config::LOGGING_ENABLED)
        {
            Logging::LogInstruction(loggedPC, decodedInstruction_->GetMnemonic(), regString);
        }
    }

    decodedInstruction_.reset();

    if (flushPipeline_)
    {
        flushPipeline_ = false;
        fetchedInstructions_ = std::queue<uint32_t>();
        return;
    }

    // Fetch
    if (fetchedInstructions_.empty())
    {
        fetchedInstructions_.push(ReadMemory(registers_.GetPC(), accessSize).first);
        registers_.AdvancePC();
    }
    else
    {
        undecodedInstruction = fetchedInstructions_.front();
        fetchedInstructions_.pop();

        fetchedInstructions_.push(ReadMemory(registers_.GetPC(), accessSize).first);
        registers_.AdvancePC();
    }

    // Decode
    if (undecodedInstruction != MAX_U32)
    {
        decodedInstruction_.reset();

        if (isArmState)
        {
            decodedInstruction_ = ARM::DecodeInstruction(undecodedInstruction);
        }
        else
        {
            decodedInstruction_ = THUMB::DecodeInstruction(undecodedInstruction);
        }
    }
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
