#include <Audio/Channel1.hpp>
#include <algorithm>
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
Channel1::Channel1() :
    channel1Registers_(),
    sound1cnt_l_(*reinterpret_cast<SOUND1CNT_L*>(&channel1Registers_[0])),
    sound1cnt_h_(*reinterpret_cast<SOUND1CNT_H*>(&channel1Registers_[2])),
    sound1cnt_x_(*reinterpret_cast<SOUND1CNT_X*>(&channel1Registers_[4]))
{
    Scheduler.RegisterEvent(EventType::Channel1Clock, std::bind(&Clock, this, std::placeholders::_1));
    Scheduler.RegisterEvent(EventType::Channel1Envelope, std::bind(&Envelope, this, std::placeholders::_1));
    Scheduler.RegisterEvent(EventType::Channel1LengthTimer, std::bind(&LengthTimer, this, std::placeholders::_1));
    Scheduler.RegisterEvent(EventType::Channel1FrequencySweep, std::bind(&FrequencySweep, this, std::placeholders::_1));
}

void Channel1::Reset()
{
    channel1Registers_.fill(0);
    envelopeIncrease_ = false;
    envelopePace_ = 0;
    currentVolume_ = 0;
    dutyCycleIndex_ = 0;
    lengthTimerExpired_ = false;
    frequencyOverflow_ = false;

    Scheduler.UnscheduleEvent(EventType::Channel1Clock);
    Scheduler.UnscheduleEvent(EventType::Channel1Envelope);
    Scheduler.UnscheduleEvent(EventType::Channel1LengthTimer);
    Scheduler.UnscheduleEvent(EventType::Channel1FrequencySweep);
}

std::pair<uint32_t, bool> Channel1::ReadReg(uint32_t addr, AccessSize alignment)
{
    if (((addr == 0x0400'0064) && (alignment == AccessSize::WORD)) || (addr >= 0x0400'0066))
    {
        return {0, false};
    }

    size_t index = addr - CHANNEL_1_ADDR_MIN;
    uint8_t* bytePtr = &channel1Registers_.at(index);
    uint32_t value = ReadPointer(bytePtr, alignment);
    return {value, false};
}

bool Channel1::WriteReg(uint32_t addr, uint32_t value, AccessSize alignment)
{
    size_t index = addr - CHANNEL_1_ADDR_MIN;
    uint8_t* bytePtr = &channel1Registers_.at(index);
    WritePointer(bytePtr, value, alignment);

    bool triggered = sound1cnt_x_.trigger;

    if (triggered)
    {
        sound1cnt_x_.trigger = 0;
        Start();
    }

    return triggered;
}

uint8_t Channel1::Sample()
{
    if (lengthTimerExpired_ || frequencyOverflow_)
    {
        return 0;
    }

    return currentVolume_ * DUTY_CYCLE[sound1cnt_h_.waveDuty][dutyCycleIndex_];
}

void Channel1::Start()
{
    // Set latched registers
    envelopeIncrease_ = sound1cnt_h_.direction;
    envelopePace_ = sound1cnt_h_.pace;

    // Set initial status
    currentVolume_ = sound1cnt_h_.initialVolume;
    dutyCycleIndex_ = 0;
    lengthTimerExpired_ = false;
    frequencyOverflow_ = false;

    // Unschedule any existing events
    Scheduler.UnscheduleEvent(EventType::Channel1Clock);
    Scheduler.UnscheduleEvent(EventType::Channel1Envelope);
    Scheduler.UnscheduleEvent(EventType::Channel1LengthTimer);
    Scheduler.UnscheduleEvent(EventType::Channel1FrequencySweep);

    // Schedule relevant events
    Scheduler.ScheduleEvent(EventType::Channel1Clock, ((0x800 - sound1cnt_x_.period) * CPU_CYCLES_PER_GB_CYCLE));

    if (sound1cnt_h_.pace != 0)
    {
        Scheduler.ScheduleEvent(EventType::Channel1Envelope, envelopePace_ * CPU_CYCLES_PER_ENVELOPE_SWEEP);
    }

    if (sound1cnt_x_.lengthEnable)
    {
        int cyclesUntilEvent = (64 - sound1cnt_h_.initialLengthTimer) * CPU_CYCLES_PER_SOUND_LENGTH;
        Scheduler.ScheduleEvent(EventType::Channel1LengthTimer, cyclesUntilEvent);
    }

    uint8_t sweepPace = std::max(sound1cnt_l_.pace, static_cast<uint16_t>(1));
    Scheduler.ScheduleEvent(EventType::Channel1FrequencySweep, sweepPace * CPU_CYCLES_PER_FREQUENCY_SWEEP);
}

void Channel1::Clock(int extraCycles)
{
    if (lengthTimerExpired_ || frequencyOverflow_)
    {
        return;
    }

    dutyCycleIndex_ = (dutyCycleIndex_ + 1) % 8;
    int cyclesUntilNextEvent =  ((0x800 - sound1cnt_x_.period) * CPU_CYCLES_PER_GB_CYCLE) - extraCycles;
    Scheduler.ScheduleEvent(EventType::Channel1Clock, cyclesUntilNextEvent);
}

void Channel1::Envelope(int extraCycles)
{
    if (lengthTimerExpired_ || frequencyOverflow_)
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
        Scheduler.ScheduleEvent(EventType::Channel1Envelope, cyclesUntilNextEvent);
    }
}

void Channel1::FrequencySweep(int extraCycles)
{
    if (lengthTimerExpired_ || frequencyOverflow_)
    {
        return;
    }

    uint16_t currentPeriod = sound1cnt_x_.period;
    uint16_t delta = currentPeriod / (0x01 << sound1cnt_l_.step);
    uint16_t updatedPeriod = 0;

    if (sound1cnt_l_.direction)
    {
        // Subtraction (period decrease)
        if (currentPeriod > delta)
        {
            updatedPeriod = currentPeriod - delta;
        }
    }
    else
    {
        // Addition (period increase)
        updatedPeriod += delta;

        if (updatedPeriod > 0x07FF)
        {
            frequencyOverflow_ = true;
            updatedPeriod = currentPeriod;
        }
    }

    uint8_t sweepPace = sound1cnt_l_.pace;

    if (sweepPace != 0)
    {
        sound1cnt_x_.period = updatedPeriod;
    }

    sweepPace = std::max(sweepPace, static_cast<uint8_t>(1));

    if (!frequencyOverflow_)
    {
        int cyclesUntilNextEvent = (sweepPace * CPU_CYCLES_PER_FREQUENCY_SWEEP) - extraCycles;
        Scheduler.ScheduleEvent(EventType::Channel1FrequencySweep, cyclesUntilNextEvent);
    }
}
}
