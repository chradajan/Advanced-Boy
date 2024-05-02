#include <ARM7TDMI/ARM7TDMI.hpp>
#include <array>
#include <cstdint>
#include <format>
#include <functional>
#include <stdexcept>
#include <string>
#include <ARM7TDMI/ArmInstructions.hpp>
#include <ARM7TDMI/CpuTypes.hpp>
#include <ARM7TDMI/ThumbInstructions.hpp>
#include <Config.hpp>
#include <Logging/Logging.hpp>
#include <System/MemoryMap.hpp>
#include <System/Scheduler.hpp>

namespace CPU
{
ARM7TDMI::ARM7TDMI(std::function<std::pair<uint32_t, int>(uint32_t, AccessSize)> ReadMemory,
                   std::function<int(uint32_t, uint32_t, AccessSize)> WriteMemory,
                   bool biosLoaded) :
    flushPipeline_(false),
    ReadMemory(ReadMemory),
    WriteMemory(WriteMemory),
    halted_(false)
{
    Scheduler.RegisterEvent(EventType::IRQ, std::bind(&IRQ, this, std::placeholders::_1));
    Scheduler.RegisterEvent(EventType::Halt, std::bind(&HALT, this, std::placeholders::_1));

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

    bool isArmState = registers_.GetOperatingState() == OperatingState::ARM;
    AccessSize alignment = isArmState ? AccessSize::WORD : AccessSize::HALFWORD;
    int cycles = 1;

    // Fetch
    uint32_t fetchPC = registers_.GetPC();
    uint32_t fetchInstruction = MAX_U32;
    std::tie(fetchInstruction, cycles) = ReadMemory(fetchPC, alignment);
    pipeline_.Push(fetchInstruction, fetchPC);

    // Decode and Execute
    if (pipeline_.Full())
    {
        auto [rawInstruction, executedPC] = pipeline_.Pop();
        std::string regString = Config::LOGGING_ENABLED ? registers_.GetRegistersString() : "";
        Instruction* instruction  = nullptr;

        if (isArmState)
        {
            instruction = ARM::DecodeInstruction(rawInstruction, *this);
        }
        else
        {
            instruction = THUMB::DecodeInstruction(rawInstruction, *this);
        }

        if (instruction != nullptr)
        {
            cycles = instruction->Execute(*this);
        }
        else
        {
            throw std::runtime_error(std::format("Unable to decode instruction {:08X} @ PC {:08X}", rawInstruction, executedPC));
        }

        if (Config::LOGGING_ENABLED)
        {
            Logging::LogMgr.LogInstruction(executedPC, instruction->GetMnemonic(), regString);
        }
    }

    // Advance or flush
    if (flushPipeline_)
    {
        pipeline_.Clear();
        flushPipeline_ = false;
    }
    else
    {
        registers_.AdvancePC();
    }

    return cycles;
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
        uint32_t savedPC = 0;

        if (pipeline_.Empty())
        {
            savedPC = registers_.GetPC();
        }
        else
        {
            savedPC = pipeline_.Front().second;
        }

        savedPC += 4;

        if (Config::LOGGING_ENABLED)
        {
            Logging::LogMgr.LogIRQ(savedPC);
        }

        registers_.SetOperatingState(OperatingState::ARM);
        registers_.SetOperatingMode(OperatingMode::IRQ);
        registers_.WriteRegister(LR_INDEX, savedPC);
        registers_.SetIrqDisabled(true);
        registers_.SetSPSR(currentCPSR);
        registers_.SetPC(0x0000'0018);
        pipeline_.Clear();
        halted_ = false;
    }
}

void ARM7TDMI::InstructionPipeline::Clear()
{
    front_ = 0;
    insertionPoint_ = 0;
    count_ = 0;
    fetchedInstructions_.fill({0xFFFF'FFFF, 0xFFFF'FFFF});
}

void ARM7TDMI::InstructionPipeline::Push(uint32_t instruction, uint32_t pc)
{
    fetchedInstructions_[insertionPoint_] = {instruction, pc};
    insertionPoint_ = (insertionPoint_ + 1) % fetchedInstructions_.size();
    ++count_;
}

std::pair<uint32_t, uint32_t> ARM7TDMI::InstructionPipeline::Pop()
{
    auto instruction = fetchedInstructions_[front_];
    front_ = (front_ + 1) % fetchedInstructions_.size();
    --count_;
    return instruction;
}
}
