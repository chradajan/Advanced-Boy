#pragma once

#include <array>
#include <cstdint>
#include <System/MemoryMap.hpp>
#include <Timers/Timer.hpp>

constexpr uint32_t TIMER_0_ADDR_MAX = 0x0400'0103;
constexpr uint32_t TIMER_1_ADDR_MAX = 0x0400'0107;
constexpr uint32_t TIMER_2_ADDR_MAX = 0x0400'010B;
constexpr uint32_t TIMER_3_ADDR_MAX = 0x0400'010F;

class TimerManager
{
public:
    /// @brief Initialize the timers and set scheduler callbacks for timer overflows.
    TimerManager();

    /// @brief Read a memory mapped timer register.
    /// @param addr Address of memory mapped register.
    /// @param alignment BYTE, HALFWORD, or WORD.
    /// @return Value of specified register.
    uint32_t ReadIoReg(uint32_t addr, AccessSize alignment);

    /// @brief Write a memory mapped timer register.
    /// @param addr Address of memory mapped register.
    /// @param value Value to write to register.
    /// @param alignment BYTE, HALFWORD, or WORD.
    void WriteIoReg(uint32_t addr, uint32_t value, AccessSize alignment);

private:
    /// @brief Callback function for a timer 0 overflow event.
    /// @param extraCycles Number of cycles that passed since this event was supposed to execute.
    void Timer0Overflow(int extraCycles);

    /// @brief Callback function for a timer 1 overflow event.
    /// @param extraCycles Number of cycles that passed since this event was supposed to execute.
    void Timer1Overflow(int extraCycles);

    /// @brief Callback function for a timer 2 overflow event.
    /// @param extraCycles Number of cycles that passed since this event was supposed to execute.
    void Timer2Overflow(int extraCycles);

    /// @brief Callback function for a timer 3 overflow event.
    /// @param extraCycles Number of cycles that passed since this event was supposed to execute.
    void Timer3Overflow(int extraCycles);

    std::array<Timer, 4> timers_;
};