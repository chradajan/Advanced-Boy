#include <Timers/Timer.hpp>
#include <array>
#include <cstdint>
#include <optional>
#include <System/EventScheduler.hpp>
#include <System/SystemControl.hpp>
#include <Utilities/MemoryUtilities.hpp>

Timer::Timer(int index, EventType overflowEvent, InterruptType interruptType) :
    timerRegisters_(),
    timerReload_(*reinterpret_cast<uint16_t*>(&timerRegisters_[0])),
    timerControl_(*reinterpret_cast<TIMCNT*>(&timerRegisters_[2])),
    timerIndex_(index),
    overflowEvent_(overflowEvent),
    interruptType_(interruptType)
{
}

void Timer::Reset()
{
    timerRegisters_.fill(0);
    internalTimer_ = 0;
}

uint32_t Timer::ReadReg(uint32_t addr, AccessSize alignment)
{
    uint32_t value = 0;
    size_t index = addr & 0x03;

    if (index < 2)  // Reading internal counter and possibly control.
    {
        if (!CascadeMode() && timerControl_.start)
        {
            UpdateInternalCounter(GetDivider(timerControl_.prescalerSelection));
        }

        switch (alignment)
        {
            case AccessSize::BYTE:
                value = ((index == 0) ? (internalTimer_ & MAX_U8) : ((internalTimer_ >> 8) & MAX_U8));
                break;
            case AccessSize::HALFWORD:
                value = internalTimer_;
                break;
            case AccessSize::WORD:
                value = (timerControl_.value << 16) | internalTimer_;
                break;
        }
    }
    else  // Reading control (must be byte or halfword aligned)
    {
        uint8_t* bytePtr = &timerRegisters_.at(index);
        value = ReadPointer(bytePtr, alignment);
    }

    return value;
}

void Timer::WriteReg(uint32_t addr, uint32_t value, AccessSize alignment)
{
    if (!CascadeMode() && timerControl_.start)
    {
        UpdateInternalCounter(GetDivider(timerControl_.prescalerSelection));
    }

    TIMCNT prevControl = timerControl_;

    size_t index = addr & 0x03;
    uint8_t* bytePtr = &timerRegisters_.at(index);
    WritePointer(bytePtr, value, alignment);

    if (!prevControl.start && timerControl_.start)  // Starting the timer
    {
        StartTimer();
    }
    else if (prevControl.start && !timerControl_.start)  // Stopping the timer
    {
        Scheduler.UnscheduleEvent(overflowEvent_);
    }
    else if (prevControl.start && timerControl_.start)  // Timer was and still is running
    {
        if (prevControl.countUpTiming && !timerControl_.countUpTiming) // Switched from cascade to normal
        {
            StartTimer();
        }
        else if (!prevControl.countUpTiming && timerControl_.countUpTiming)  // Switch from normal to cascade
        {
            Scheduler.UnscheduleEvent(overflowEvent_);
        }
    }
}

void Timer::StartTimer()
{
    internalTimer_ = timerReload_;

    if (!CascadeMode())
    {
        uint64_t cyclesUntilOverflow = ((0x0001'0000 - internalTimer_) * GetDivider(timerControl_.prescalerSelection)) + 2;
        Scheduler.ScheduleEvent(overflowEvent_, cyclesUntilOverflow);
    }
}

int Timer::Overflow(int extraCycles)
{
    int overflowCount = 1;
    internalTimer_ = timerReload_;

    if (!CascadeMode())
    {
        uint16_t divider = GetDivider(timerControl_.prescalerSelection);
        uint64_t timerDuration = (0x0001'0000 - timerReload_) * divider;

        if (timerDuration <= static_cast<uint64_t>(extraCycles))
        {
            overflowCount += extraCycles / timerDuration;
            extraCycles %= timerDuration;
        }

        internalTimer_ += (extraCycles / divider);
        extraCycles %= divider;

        int cyclesUntilOverflow = ((0x0001'0000 - internalTimer_) * divider) - extraCycles;
        Scheduler.ScheduleEvent(overflowEvent_, cyclesUntilOverflow);
    }

    return overflowCount;
}

void Timer::CascadeModeIncrement(int incrementCount)
{
    if ((internalTimer_ + incrementCount) > 0xFFFF)
    {
        Scheduler.ScheduleEvent(overflowEvent_, SCHEDULE_NOW);
    }
    else
    {
        internalTimer_ += incrementCount;
    }
}

void Timer::UpdateInternalCounter(uint16_t divider)
{
    auto elapsedCycles = Scheduler.ElapsedCycles(overflowEvent_);

    if (elapsedCycles.has_value())
    {
        internalTimer_ += (elapsedCycles.value() / divider);
    }
}

uint16_t Timer::GetDivider(uint16_t prescalerSelection) const
{
    switch (prescalerSelection)
    {
        case 0:
            return 1;
        case 1:
            return 64;
        case 2:
            return 256;
        case 3:
            return 1024;
    }

    return 1;
}
