#include <Timers/TimerManager.hpp>
#include <array>
#include <cstdint>
#include <utility>
#include <System/EventScheduler.hpp>
#include <System/SystemControl.hpp>
#include <Utilities/MemoryUtilities.hpp>
#include <Timers/Timer.hpp>

TimerManager::TimerManager() :
    timers_({Timer(0, EventType::Timer0Overflow, InterruptType::TIMER_0_OVERFLOW),
             Timer(1, EventType::Timer1Overflow, InterruptType::TIMER_1_OVERFLOW),
             Timer(2, EventType::Timer2Overflow, InterruptType::TIMER_2_OVERFLOW),
             Timer(3, EventType::Timer3Overflow, InterruptType::TIMER_3_OVERFLOW)})
{
    Scheduler.RegisterEvent(EventType::Timer2Overflow, std::bind(&Timer2Overflow, this, std::placeholders::_1));
    Scheduler.RegisterEvent(EventType::Timer3Overflow, std::bind(&Timer3Overflow, this, std::placeholders::_1));
}

void TimerManager::Reset()
{
    for (auto& timer : timers_)
    {
        timer.Reset();
    }
}

std::pair<uint32_t, bool> TimerManager::ReadReg(uint32_t addr, AccessSize alignment)
{
    uint32_t value = 0;

    if (addr < 0x0400'0104)
    {
        value = timers_[0].ReadReg(addr, alignment);
    }
    else if (addr < 0x0400'0108)
    {
        value = timers_[1].ReadReg(addr, alignment);
    }
    else if (addr < 0x0400'010C)
    {
        value = timers_[2].ReadReg(addr, alignment);
    }
    else if (addr < 0x0400'0110)
    {
        value = timers_[3].ReadReg(addr, alignment);
    }

    return {value, false};
}

void TimerManager::WriteReg(uint32_t addr, uint32_t value, AccessSize alignment)
{
    if (addr < 0x0400'0104)
    {
        timers_[0].WriteReg(addr, value, alignment);
    }
    else if (addr < 0x0400'0108)
    {
        timers_[1].WriteReg(addr, value, alignment);
    }
    else if (addr < 0x0400'010C)
    {
        timers_[2].WriteReg(addr, value, alignment);
    }
    else if (addr < 0x0400'0110)
    {
        timers_[3].WriteReg(addr, value, alignment);
    }
}

void TimerManager::TimerOverflow(int timerIndex, int extraCycles)
{
    Timer& overflowTimer = timers_[timerIndex];
    Timer* nextTimer = nullptr;

    if (timerIndex < 3)
    {
        nextTimer = &timers_[timerIndex + 1];
    }

    if (overflowTimer.GenerateIRQ())
    {
        SystemController.RequestInterrupt(overflowTimer.GetInterruptType());
    }

    int overflowCount = overflowTimer.Overflow(extraCycles);

    if ((nextTimer != nullptr) && (nextTimer->CascadeMode()))
    {
        nextTimer->CascadeModeIncrement(overflowCount);
    }
}
