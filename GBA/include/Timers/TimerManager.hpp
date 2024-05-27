#pragma once

#include <array>
#include <cstdint>
#include <utility>
#include <Timers/Timer.hpp>
#include <Utilities/MemoryUtilities.hpp>

class TimerManager
{
public:
    /// @brief Initialize the timers and set scheduler callbacks for timer overflows.
    TimerManager();

    /// @brief Reset the timers to their power-up state.
    void Reset();

    /// @brief Read a memory mapped timer register.
    /// @param addr Address of memory mapped register.
    /// @param alignment Number of bytes to read.
    /// @return Value of specified register and whether this read triggered open bus behavior.
    std::pair<uint32_t, bool> ReadReg(uint32_t addr, AccessSize alignment);

    /// @brief Write a memory mapped timer register.
    /// @param addr Address of memory mapped register.
    /// @param value Value to write to register.
    /// @param alignment Number of bytes to write.
    void WriteReg(uint32_t addr, uint32_t value, AccessSize alignment);

    /// @brief Callback function for a timer 0 overflow event.
    /// @param extraCycles Number of cycles that passed since this event was supposed to execute.
    void Timer0Overflow(int extraCycles) { TimerOverflow(0, extraCycles); }

    /// @brief Callback function for a timer 1 overflow event.
    /// @param extraCycles Number of cycles that passed since this event was supposed to execute.
    void Timer1Overflow(int extraCycles) { TimerOverflow(1, extraCycles); }

private:
    /// @brief Callback function for a timer 2 overflow event.
    /// @param extraCycles Number of cycles that passed since this event was supposed to execute.
    void Timer2Overflow(int extraCycles) { TimerOverflow(2, extraCycles); }

    /// @brief Callback function for a timer 3 overflow event.
    /// @param extraCycles Number of cycles that passed since this event was supposed to execute.
    void Timer3Overflow(int extraCycles) { TimerOverflow(3, extraCycles); }

    /// @brief Handle a timer overflow event.
    /// @param timerIndex Index of timer that overflowed.
    /// @param extraCycles Number of cycles that passed since this event was supposed to execute.
    void TimerOverflow(int timerIndex, int extraCycles);

    std::array<Timer, 4> timers_;
};
