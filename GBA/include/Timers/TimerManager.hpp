#pragma once

#include <array>
#include <cstdint>
#include <utility>
#include <Timers/Timer.hpp>
#include <Utilities/MemoryUtilities.hpp>

constexpr uint32_t TIMER_0_ADDR_MAX = 0x0400'0103;
constexpr uint32_t TIMER_1_ADDR_MAX = 0x0400'0107;
constexpr uint32_t TIMER_2_ADDR_MAX = 0x0400'010B;
constexpr uint32_t TIMER_3_ADDR_MAX = 0x0400'010F;

class TimerManager
{
public:
    /// @brief Initialize the timers and set scheduler callbacks for timer overflows.
    TimerManager();

    /// @brief Reset the timers to their power-up state.
    void Reset();

    /// @brief Read a memory mapped timer register.
    /// @param addr Address of memory mapped register.
    /// @param alignment BYTE, HALFWORD, or WORD.
    /// @return Value of specified register.
    std::pair<uint32_t, bool> ReadReg(uint32_t addr, AccessSize alignment);

    /// @brief Write a memory mapped timer register.
    /// @param addr Address of memory mapped register.
    /// @param value Value to write to register.
    /// @param alignment BYTE, HALFWORD, or WORD.
    void WriteReg(uint32_t addr, uint32_t value, AccessSize alignment);

    /// @brief Callback function for a timer 0 overflow event.
    /// @param extraCycles Number of cycles that passed since this event was supposed to execute.
    void Timer0Overflow(int extraCycles);

    /// @brief Callback function for a timer 1 overflow event.
    /// @param extraCycles Number of cycles that passed since this event was supposed to execute.
    void Timer1Overflow(int extraCycles);

private:
    /// @brief Callback function for a timer 2 overflow event.
    /// @param extraCycles Number of cycles that passed since this event was supposed to execute.
    void Timer2Overflow(int extraCycles);

    /// @brief Callback function for a timer 3 overflow event.
    /// @param extraCycles Number of cycles that passed since this event was supposed to execute.
    void Timer3Overflow(int extraCycles);

    std::array<Timer, 4> timers_;
};
