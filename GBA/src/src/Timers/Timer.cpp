#include <Timers/Timer.hpp>
#include <array>
#include <cstdint>
#include <optional>
#include <System/Scheduler.hpp>
#include <System/Utilities.hpp>

Timer::Timer(int index) :
    timerRegisters_(),
    timerControl_(*reinterpret_cast<TIMXCNT*>(timerRegisters_.data())),
    internalTimer_(0),
    timerIndex_(index)
{
    switch (index)
    {
        case 0:
            overflowEvent_ = EventType::Timer0Overflow;
            break;
        case 1:
            overflowEvent_ = EventType::Timer1Overflow;
            break;
        case 2:
            overflowEvent_ = EventType::Timer2Overflow;
            break;
        case 3:
            overflowEvent_ = EventType::Timer3Overflow;
            break;
    }
}

uint32_t Timer::ReadIoReg(uint32_t addr, AccessSize alignment)
{
    size_t index = addr & 0x03;
    uint32_t value = 0;

    if (timerControl_.flags_.start_ && (index < 2))
    {
        UpdateInternalCounter(GetDivider());
    }

    switch (index)
    {
        case 0:
        {
            switch (alignment)
            {
                case AccessSize::BYTE:
                    value = internalTimer_ & MAX_U8;
                    break;
                case AccessSize::HALFWORD:
                    value = internalTimer_;
                    break;
                case AccessSize::WORD:
                    value = (timerControl_.word_ & 0xFFFF'0000) | internalTimer_;
                    break;
            }

            break;
        }
        case 1:
            value = (internalTimer_ >> 8) & MAX_U8;
            break;
        case 2:
        {
            switch (alignment)
            {
                case AccessSize::BYTE:
                    value = (timerControl_.word_ >> 16) & MAX_U8;
                    break;
                case AccessSize::HALFWORD:
                    value = (timerControl_.word_ >> 16) & MAX_U16;
                    break;
                default:
                    break;
            }

            break;
        }
        case 3:
            value = (timerControl_.word_ >> 24) & MAX_U8;
            break;
    }

    return value;
}

void Timer::WriteIoReg(uint32_t addr, uint32_t value, AccessSize alignment)
{
    bool wasTimerRunning = timerControl_.flags_.start_;
    bool wasTimerCountUpMode = CascadeMode();
    uint16_t oldDivider = GetDivider();

    size_t index = addr & 0x03;
    uint8_t* bytePtr = &timerRegisters_[index];
    WritePointer(bytePtr, value, alignment);

    if (wasTimerRunning && !wasTimerCountUpMode && CascadeMode())
    {
        // Timer switched from normal to cascade mode.
        UpdateInternalCounter(oldDivider, true);
        Scheduler.UnscheduleEvent(overflowEvent_);
    }
    else if (wasTimerRunning && timerControl_.flags_.start_ && wasTimerCountUpMode && !CascadeMode())
    {
        // Timer was and still is running, and it switched from cascade to normal mode.
        uint64_t cyclesUntilOverflow = ((0x0001'0000 - internalTimer_) * GetDivider());
        Scheduler.ScheduleEvent(overflowEvent_, cyclesUntilOverflow);
    }
    else if (!wasTimerRunning && timerControl_.flags_.start_)
    {
        // Timer was not running and now is.
        StartTimer();
    }
    else if (wasTimerRunning && !timerControl_.flags_.start_)
    {
        // Timer was running and is not anymore.
        StopTimer();
    }
}

void Timer::StartTimer()
{
    internalTimer_ = timerControl_.flags_.reload_;

    if (!CascadeMode())
    {
        uint64_t cyclesUntilOverflow = ((0x0001'0000 - internalTimer_) * GetDivider()) + 2;
        Scheduler.ScheduleEvent(overflowEvent_, cyclesUntilOverflow);
    }
}

int Timer::Overflow(int extraCycles)
{
    internalTimer_ = timerControl_.flags_.reload_;
    int numberOfOverflows = 1;

    if (!CascadeMode())
    {
        uint64_t timerDurationInCycles = ((0x0001'0000 - internalTimer_) * GetDivider());

        if (static_cast<uint64_t>(extraCycles) > timerDurationInCycles)
        {
            numberOfOverflows += extraCycles / timerDurationInCycles;
            extraCycles = extraCycles % timerDurationInCycles;
        }

        uint64_t cyclesUntilOverflow = timerDurationInCycles - extraCycles;
        Scheduler.ScheduleEvent(overflowEvent_, cyclesUntilOverflow);
    }

    return numberOfOverflows;
}

void Timer::CascadeModeIncrement(int incrementCount)
{
    // TODO: Improve this so that multiple cascaded increments will propagate forward if the next timer is also in cascade mode.
    if ((internalTimer_ + incrementCount) < internalTimer_)
    {
        Scheduler.ScheduleEvent(overflowEvent_, SCHEDULE_NOW);
    }

    while (incrementCount > 0)
    {
        ++internalTimer_;
        --incrementCount;

        if (internalTimer_ == 0)
        {
            internalTimer_ = timerControl_.flags_.reload_;
        }
    }
}

void Timer::StopTimer()
{
    UpdateInternalCounter(GetDivider());
    Scheduler.UnscheduleEvent(overflowEvent_);
}

void Timer::UpdateInternalCounter(uint16_t divider, bool forceUpdate)
{
    if (!CascadeMode() || forceUpdate)
    {
        auto elapsedCycles = Scheduler.ElapsedCycles(overflowEvent_);

        if (elapsedCycles.has_value())
        {
            internalTimer_ += (elapsedCycles.value() / divider);
        }
    }
}

uint16_t Timer::GetDivider() const
{
    switch (timerControl_.flags_.prescalerSelection_)
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
}
