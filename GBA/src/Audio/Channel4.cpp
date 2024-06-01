#include <Audio/Channel4.hpp>
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
Channel4::Channel4() :
    channel4Registers_(),
    sound4cnt_l_(*reinterpret_cast<SOUND4CNT_L*>(&channel4Registers_[0])),
    sound4cnt_h_(*reinterpret_cast<SOUND4CNT_H*>(&channel4Registers_[4]))
{
    Scheduler.RegisterEvent(EventType::Channel4Clock, std::bind(&Clock, this, std::placeholders::_1), true);
    Scheduler.RegisterEvent(EventType::Channel4Envelope, std::bind(&Envelope, this, std::placeholders::_1), true);
    Scheduler.RegisterEvent(EventType::Channel4LengthTimer, std::bind(&LengthTimer, this, std::placeholders::_1), true);
}

void Channel4::Reset()
{
    channel4Registers_.fill(0);
    envelopeIncrease_ = false;
    envelopePace_ = 0;
    currentVolume_ = 0;
    lengthTimerExpired_ = false;

    Scheduler.UnscheduleEvent(EventType::Channel4Clock);
    Scheduler.UnscheduleEvent(EventType::Channel4Envelope);
    Scheduler.UnscheduleEvent(EventType::Channel4LengthTimer);
}

std::pair<uint32_t, bool> Channel4::ReadReg(uint32_t addr, AccessSize alignment)
{
    if (((addr == 0x0400'0078) && (alignment == AccessSize::WORD)) ||
        ((addr == 0x0400'007C) && (alignment == AccessSize::WORD)) ||
        ((0x0400'007A <= addr) && (addr < 0x0400'007C)) ||
        ((0x0400'007E <= addr) && (addr < 0x0400'0080)))
    {
        return {0, false};
    }

    size_t index = addr - CHANNEL_4_ADDR_MIN;
    uint8_t* bytePtr = &channel4Registers_.at(index);
    uint32_t value = ReadPointer(bytePtr, alignment);
    return {value, false};
}

bool Channel4::WriteReg(uint32_t addr, uint32_t value, AccessSize alignment)
{
    size_t index = addr - CHANNEL_4_ADDR_MIN;
    uint8_t* bytePtr = &channel4Registers_.at(index);
    WritePointer(bytePtr, value, alignment);

    bool triggered = sound4cnt_h_.trigger;

    if (triggered)
    {
        sound4cnt_h_.trigger = 0;
        Start();
    }

    return triggered;
}

uint8_t Channel4::Sample()
{
    if (lengthTimerExpired_)
    {
        return 0;
    }

    return (lsfr_ & 0x0001) * currentVolume_;
}

void Channel4::Start()
{
    // Set latched registers
    envelopeIncrease_ = sound4cnt_l_.direction;
    envelopePace_ = sound4cnt_l_.pace;

    // Set initial status
    currentVolume_ = sound4cnt_l_.initialVolume;
    lengthTimerExpired_ = false;

    // Unschedule any existing events
    Scheduler.UnscheduleEvent(EventType::Channel4Clock);
    Scheduler.UnscheduleEvent(EventType::Channel4Envelope);
    Scheduler.UnscheduleEvent(EventType::Channel4LengthTimer);

    // Schedule relevant events
    Scheduler.ScheduleEvent(EventType::Channel4Clock, EventCycles());

    if (envelopePace_ != 0)
    {
        Scheduler.ScheduleEvent(EventType::Channel4Envelope, envelopePace_ * CPU_CYCLES_PER_ENVELOPE_SWEEP);
    }

    if (sound4cnt_h_.lengthEnable)
    {
        int cyclesUntilEvent = (64 - sound4cnt_l_.initialLengthTimer) * CPU_CYCLES_PER_SOUND_LENGTH;
        Scheduler.ScheduleEvent(EventType::Channel4LengthTimer, cyclesUntilEvent);
    }

    lsfr_ = 0xFFFF;
}

void Channel4::Clock(int extraCycles)
{
    if (lengthTimerExpired_)
    {
        return;
    }

    uint16_t result = (lsfr_ & 0x0001) ^ ((lsfr_ & 0x0002) >> 1);
    lsfr_ = (lsfr_ & 0x7FFF) | (result << 15);

    if (sound4cnt_h_.countWidth)
    {
        lsfr_ = (lsfr_ & 0xFF7F) | (result << 7);
    }

    lsfr_ >>= 1;
    Scheduler.ScheduleEvent(EventType::Channel4Clock, EventCycles() - extraCycles);
}

void Channel4::Envelope(int extraCycles)
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
        Scheduler.ScheduleEvent(EventType::Channel4Envelope, cyclesUntilNextEvent);
    }
}

int Channel4::EventCycles() const
{
    uint8_t r = sound4cnt_h_.dividingRatio;
    uint8_t s = sound4cnt_h_.shiftClockFrequency;
    int frequency = 1;

    if (r == 0)
    {
        frequency = 524288 / (1 << s);
    }
    else
    {
        frequency = 262144 / (r * (1 << s));
    }

    return CPU::CPU_FREQUENCY_HZ / frequency;
}
}
