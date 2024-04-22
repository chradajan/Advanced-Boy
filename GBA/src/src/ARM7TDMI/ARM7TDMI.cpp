#include <ARM7TDMI/ARM7TDMI.hpp>
#include <ARM7TDMI/ArmInstructions.hpp>
#include <ARM7TDMI/CpuTypes.hpp>
#include <ARM7TDMI/ThumbInstructions.hpp>
#include <Config.hpp>
#include <Logging/Logging.hpp>
#include <System/MemoryMap.hpp>
#include <System/Scheduler.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <queue>

namespace CPU
{
ARM7TDMI::ARM7TDMI(std::function<std::pair<uint32_t, int>(uint32_t, AccessSize)> ReadMemory,
                   std::function<int(uint32_t, uint32_t, AccessSize)> WriteMemory,
                   bool biosLoaded) :
    decodedInstruction_(nullptr),
    flushPipeline_(false),
    ReadMemory(ReadMemory),
    WriteMemory(WriteMemory),
    halted_(false)
{
    Scheduler.RegisterEvent(EventType::IRQ, std::bind(&IRQ, this, std::placeholders::_1));
    Scheduler.RegisterEvent(EventType::HALT, std::bind(&HALT, this, std::placeholders::_1));

    if (!biosLoaded)
    {
        registers_.SkipBIOS();
    }
}

int ARM7TDMI::Tick()
{
    if (halted_)
    {
        return 1;
    }

    int executionCycles = 1;
    int fetchCycles = 1;
    bool instructionExecuted = false;
    bool const isArmState = registers_.GetOperatingState() == OperatingState::ARM;
    AccessSize alignment = isArmState ? AccessSize::WORD : AccessSize::HALFWORD;
    uint32_t undecodedInstruction = MAX_U32;

    // Execute
    if (decodedInstruction_)
    {
        uint32_t loggedPC;
        std::string regString;

        if (Config::LOGGING_ENABLED)
        {
            loggedPC = isArmState ? registers_.GetPC() - 8 : registers_.GetPC() - 4;
            regString = registers_.GetRegistersString();
        }

        try
        {
            executionCycles = decodedInstruction_->Execute(*this);
        }
        catch (std::runtime_error const& error)
        {
            if (Config::LOGGING_ENABLED)
            {
                Logging::LogMgr.LogInstruction(loggedPC, decodedInstruction_->GetMnemonic(), regString);
            }

            throw;
        }

        instructionExecuted = true;

        if (Config::LOGGING_ENABLED)
        {
            Logging::LogMgr.LogInstruction(loggedPC, decodedInstruction_->GetMnemonic(), regString);
        }
    }

    decodedInstruction_.reset();

    if (flushPipeline_)
    {
        flushPipeline_ = false;
        fetchedInstructions_ = std::queue<uint32_t>();
        return executionCycles;
    }

    // Fetch
    if (fetchedInstructions_.empty())
    {
        auto [fetchedInstruction, readCycles] = ReadMemory(registers_.GetPC(), alignment);
        fetchCycles = readCycles;
        fetchedInstructions_.push(fetchedInstruction);
        registers_.AdvancePC();
    }
    else
    {
        undecodedInstruction = fetchedInstructions_.front();
        fetchedInstructions_.pop();

        auto [fetchedInstruction, readCycles] = ReadMemory(registers_.GetPC(), alignment);
        fetchCycles = readCycles;
        fetchedInstructions_.push(fetchedInstruction);
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

    return instructionExecuted ? executionCycles : fetchCycles;
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
            return !registers_.IsCarry() || registers_.IsZero();
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

void ARM7TDMI::IRQ(int)
{
    if (!registers_.IsIrqDisabled())
    {
        uint32_t currentCPSR = registers_.GetCPSR();
        uint32_t savedPC = registers_.GetPC() - ((registers_.GetOperatingState() == OperatingState::ARM) ? 4 : 2);
        registers_.SetOperatingState(OperatingState::ARM);
        registers_.SetOperatingMode(OperatingMode::IRQ);
        registers_.WriteRegister(LR_INDEX, savedPC);
        registers_.SetIrqDisabled(true);
        registers_.SetSPSR(currentCPSR);
        registers_.SetPC(0x0000'0018);
        decodedInstruction_.reset();
        fetchedInstructions_ = std::queue<uint32_t>();
        halted_ = false;
    }
}
}
