#include <Audio/Channel2.hpp>
#include <array>
#include <cstdint>
#include <functional>
#include <utility>
#include <Audio/Constants.hpp>
#include <Audio/Registers.hpp>
#include <System/EventScheduler.hpp>
#include <System/MemoryMap.hpp>
#include <Utilities/MemoryUtilities.hpp>

namespace Audio
{
Channel2::Channel2() :
    channel2Registers_(),
    sound2cnt_l_(*reinterpret_cast<SOUND2CNT_L*>(&channel2Registers_[0])),
    sound2cnt_h_(*reinterpret_cast<SOUND2CNT_H*>(&channel2Registers_[4]))
{
    Scheduler.RegisterEvent(EventType::Channel2Clock, std::bind(&Clock, this, std::placeholders::_1));
    Scheduler.RegisterEvent(EventType::Channel2Envelope, std::bind(&Envelope, this, std::placeholders::_1));
    Scheduler.RegisterEvent(EventType::Channel2LengthTimer, std::bind(&LengthTimer, this, std::placeholders::_1));
}

void Channel2::Reset()
{
    channel2Registers_.fill(0);
    envelopeIncrease_ = false;
    envelopePace_ = 0;
    currentVolume_ = 0;
    dutyCycleIndex_ = 0;
    lengthTimerExpired_ = false;

    Scheduler.UnscheduleEvent(EventType::Channel2Clock);
    Scheduler.UnscheduleEvent(EventType::Channel2Envelope);
    Scheduler.UnscheduleEvent(EventType::Channel2LengthTimer);
}

std::pair<uint32_t, bool> Channel2::ReadReg(uint32_t addr, AccessSize alignment)
{
    if (((addr == 0x0400'0068) && (alignment == AccessSize::WORD)) ||
        ((addr == 0x0400'006C) && (alignment == AccessSize::WORD)) ||
        ((0x0400'006A <= addr) && (addr < 0x0400'006C)) ||
        ((0x0400'006E <= addr) && (addr < 0x0400'0070)))
    {
        return {0, false};
    }

    size_t index = addr - CHANNEL_2_ADDR_MIN;
    uint8_t* bytePtr = &channel2Registers_.at(index);
    uint32_t value = ReadPointer(bytePtr, alignment);
    return {value, false};
}

bool Channel2::WriteReg(uint32_t addr, uint32_t value, AccessSize alignment)
{
    size_t index = addr - CHANNEL_2_ADDR_MIN;
    uint8_t* bytePtr = &channel2Registers_.at(index);
    WritePointer(bytePtr, value, alignment);

    bool triggered = sound2cnt_h_.trigger;

    if (triggered)
    {
        sound2cnt_h_.trigger = 0;
        Start();
    }

    return triggered;
}

uint8_t Channel2::Sample()
{
    if (lengthTimerExpired_)
    {
        return 0;
    }

    return currentVolume_ * DUTY_CYCLE[sound2cnt_l_.waveDuty][dutyCycleIndex_];
}

void Channel2::Start()
{
    // Set latched registers
    envelopeIncrease_ = sound2cnt_l_.direction;
    envelopePace_ = sound2cnt_l_.pace;

    // Set initial status
    currentVolume_ = sound2cnt_l_.initialVolume;
    dutyCycleIndex_ = 0;
    lengthTimerExpired_ = false;

    // Unschedule any existing events
    Scheduler.UnscheduleEvent(EventType::Channel2Clock);
    Scheduler.UnscheduleEvent(EventType::Channel2Envelope);
    Scheduler.UnscheduleEvent(EventType::Channel2LengthTimer);

    // Schedule relevant events
    Scheduler.ScheduleEvent(EventType::Channel2Clock, ((0x800 - sound2cnt_h_.period) * CPU_CYCLES_PER_GB_CYCLE));

    if (sound2cnt_l_.pace != 0)
    {
        Scheduler.ScheduleEvent(EventType::Channel2Envelope, envelopePace_ * CPU_CYCLES_PER_ENVELOPE_SWEEP);
    }

    if (sound2cnt_h_.lengthEnable)
    {
        int cyclesUntilEvent = (64 - sound2cnt_l_.initialLengthTimer) * CPU_CYCLES_PER_SOUND_LENGTH;
        Scheduler.ScheduleEvent(EventType::Channel2LengthTimer, cyclesUntilEvent);
    }
}

void Channel2::Clock(int extraCycles)
{
    if (lengthTimerExpired_)
    {
        return;
    }

    dutyCycleIndex_ = (dutyCycleIndex_ + 1) % 8;
    int cyclesUntilNextEvent =  ((0x800 - sound2cnt_h_.period) * CPU_CYCLES_PER_GB_CYCLE) - extraCycles;
    Scheduler.ScheduleEvent(EventType::Channel2Clock, cyclesUntilNextEvent);
}

void Channel2::Envelope(int extraCycles)
{
    if (lengthTimerExpired_)
    {
        return;
    }

    bool reschedule = true;

    if (envelopeIncrease_ && (currentVolume_ < 0x0F))
    {
        ++currentVolume_;
    }
    else if (!envelopeIncrease_ && (currentVolume_ > 0))
    {
        --currentVolume_;
    }
    else
    {
        reschedule = false;
    }

    if (reschedule)
    {
        int cyclesUntilNextEvent = (envelopePace_ * CPU_CYCLES_PER_ENVELOPE_SWEEP) - extraCycles;
        Scheduler.ScheduleEvent(EventType::Channel2Envelope, cyclesUntilNextEvent);
    }
}
}
