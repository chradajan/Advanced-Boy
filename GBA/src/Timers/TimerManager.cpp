#include <Timers/TimerManager.hpp>
#include <array>
#include <cstdint>
#include <utility>
#include <System/MemoryMap.hpp>
#include <System/SystemControl.hpp>
#include <System/EventScheduler.hpp>
#include <Utilities/MemoryUtilities.hpp>
#include <Timers/Timer.hpp>

TimerManager::TimerManager() :
    timers_({0, 1, 2, 3})
{
    Scheduler.RegisterEvent(EventType::Timer2Overflow, std::bind(&Timer2Overflow, this, std::placeholders::_1), true);
    Scheduler.RegisterEvent(EventType::Timer3Overflow, std::bind(&Timer3Overflow, this, std::placeholders::_1), true);
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

    if (addr <= TIMER_0_ADDR_MAX)
    {
        value = timers_[0].ReadIoReg(addr, alignment);
    }
    else if (addr <= TIMER_1_ADDR_MAX)
    {
        value = timers_[1].ReadIoReg(addr, alignment);
    }
    else if (addr <= TIMER_2_ADDR_MAX)
    {
        value = timers_[2].ReadIoReg(addr, alignment);
    }
    else if (addr <= TIMER_3_ADDR_MAX)
    {
        value = timers_[3].ReadIoReg(addr, alignment);
    }

    return {value, false};
}

void TimerManager::WriteReg(uint32_t addr, uint32_t value, AccessSize alignment)
{
    if (addr <= TIMER_0_ADDR_MAX)
    {
        timers_[0].WriteIoReg(addr, value, alignment);
    }
    else if (addr <= TIMER_1_ADDR_MAX)
    {
        timers_[1].WriteIoReg(addr, value, alignment);
    }
    else if (addr <= TIMER_2_ADDR_MAX)
    {
        timers_[2].WriteIoReg(addr, value, alignment);
    }
    else if (addr <= TIMER_3_ADDR_MAX)
    {
        timers_[3].WriteIoReg(addr, value, alignment);
    }
}

void TimerManager::Timer0Overflow(int extraCycles)
{
    if (timers_[0].GenerateIRQ())
    {
        SystemController.RequestInterrupt(InterruptType::TIMER_0_OVERFLOW);
    }

    int numberOfOverflows = timers_[0].Overflow(extraCycles);

    if (timers_[1].CascadeMode())
    {
        timers_[1].CascadeModeIncrement(numberOfOverflows);
    }
}

void TimerManager::Timer1Overflow(int extraCycles)
{
    if (timers_[1].GenerateIRQ())
    {
        SystemController.RequestInterrupt(InterruptType::TIMER_1_OVERFLOW);
    }

    int numberOfOverflows = timers_[1].Overflow(extraCycles);

    if (timers_[2].CascadeMode())
    {
        timers_[2].CascadeModeIncrement(numberOfOverflows);
    }
}

void TimerManager::Timer2Overflow(int extraCycles)
{
    if (timers_[2].GenerateIRQ())
    {
        SystemController.RequestInterrupt(InterruptType::TIMER_2_OVERFLOW);
    }

    int numberOfOverflows = timers_[2].Overflow(extraCycles);

    if (timers_[3].CascadeMode())
    {
        timers_[3].CascadeModeIncrement(numberOfOverflows);
    }
}

void TimerManager::Timer3Overflow(int extraCycles)
{
    if (timers_[3].GenerateIRQ())
    {
        SystemController.RequestInterrupt(InterruptType::TIMER_3_OVERFLOW);
    }

    timers_[3].Overflow(extraCycles);
}
