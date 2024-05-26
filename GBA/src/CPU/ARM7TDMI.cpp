#include <CPU/ARM7TDMI.hpp>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <CPU/ArmInstructions.hpp>
#include <CPU/CPUTypes.hpp>
#include <CPU/ThumbInstructions.hpp>
#include <Logging/Logging.hpp>
#include <System/EventScheduler.hpp>
#include <Utilities/CircularBuffer.hpp>
#include <Utilities/Functor.hpp>
#include <Utilities/MemoryUtilities.hpp>

namespace CPU
{
using namespace Logging;

ARM7TDMI::ARM7TDMI(MemReadCallback readCallback, MemWriteCallback writeCallback) :
    ReadMemory(readCallback),
    WriteMemory(writeCallback),
    flushPipeline_(false)
{
    Scheduler.RegisterEvent(EventType::IRQ, std::bind(&IRQ, this, std::placeholders::_1), false);
}

void ARM7TDMI::Reset()
{
    pipeline_.Clear();
    flushPipeline_ = false;
    registers_ = CPU::Registers();
}

void ARM7TDMI::Step()
{
    bool armMode = registers_.GetOperatingState() == OperatingState::ARM;
    AccessSize alignment = armMode ? AccessSize::WORD : AccessSize::HALFWORD;

    // Fetch
    uint32_t fetchedPC = registers_.GetPC();
    auto [fetchedInstruction, cycles] = ReadMemory(fetchedPC, alignment);
    pipeline_.Push({fetchedInstruction, fetchedPC});
    Scheduler.Step(cycles);

    // Decode and execute
    if (pipeline_.Full())
    {
        auto [undecodedInstruction, executedPC] = pipeline_.Pop();

        if (LogMgr.LoggingEnabled())
        {
            registers_.SetRegistersString(regString_);
        }

        Instruction* instruction = nullptr;

        if (armMode)
        {
            instruction = ARM::DecodeInstruction(undecodedInstruction, instructionBuffer_);
        }
        else
        {
            instruction = THUMB::DecodeInstruction(undecodedInstruction, instructionBuffer_);
        }

        if (instruction != nullptr)
        {
            instruction->Execute(*this);
        }
        else
        {
            throw std::runtime_error("Unable to decode instruction");
        }

        if (LogMgr.LoggingEnabled())
        {
            LogMgr.LogInstruction(executedPC, mnemonic_, regString_);
        }
    }

    if (flushPipeline_)
    {
        pipeline_.Clear();
        flushPipeline_ = false;
    }
    else
    {
        registers_.AdvancePC();
    }
}

bool ARM7TDMI::ArmConditionSatisfied(uint8_t condition)
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
        default:
            break;
    }

    throw std::runtime_error("Illegal ARM condition");
}

void ARM7TDMI::IRQ(int)
{
    if (!registers_.IsIrqDisabled())
    {
        uint32_t currentCPSR = registers_.GetCPSR();
        uint32_t savedPC = 0;

        if (pipeline_.Empty())
        {
            savedPC = registers_.GetPC();
        }
        else
        {
            savedPC = pipeline_.Peak().second;
        }

        savedPC += 4;

        if (LogMgr.LoggingEnabled())
        {
            LogMgr.LogIRQ();
        }

        registers_.SetOperatingState(OperatingState::ARM);
        registers_.SetOperatingMode(OperatingMode::IRQ);
        registers_.WriteRegister(LR_INDEX, savedPC);
        registers_.SetIrqDisabled(true);
        registers_.SetSPSR(currentCPSR);
        registers_.SetPC(0x0000'0018);
        pipeline_.Clear();
    }
}
}
