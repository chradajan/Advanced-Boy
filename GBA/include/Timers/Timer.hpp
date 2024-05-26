#pragma once

#include <array>
#include <cstdint>
#include <System/MemoryMap.hpp>
#include <System/Scheduler.hpp>

class Timer
{
public:
    /// @brief Initialize a timer.
    /// @param index Which timer (0-3) this is.
    Timer(int index);

    /// @brief Read a memory mapped timer register.
    /// @param addr Address of memory mapped register.
    /// @param alignment BYTE, HALFWORD, or WORD.
    /// @return Value of specified register.
    uint32_t ReadIoReg(uint32_t addr, AccessSize alignment);

    /// @brief Read a memory mapped timer register.
    /// @param addr Address of memory mapped register.
    /// @param value Value to write to register.
    /// @param alignment BYTE, HALFWORD, or WORD.
    void WriteIoReg(uint32_t addr, uint32_t value, AccessSize alignment);

    /// @brief Reload the internal counter and schedule a timer overflow event.
    /// @param extraCycles How many cycles have passed since this timer should have started.
    /// @return How many times this timer overflowed in the duration elapsed since it should have started.
    void StartTimer();

    /// @brief Restart the timer after it overflows.
    /// @param extraCycles How many cycles have passed since this timer should have restarted.
    /// @return How many times this timer overflowed in the duration elapsed since it should have restarted.
    int Overflow(int extraCycles);

    /// @brief If this timer is in cascade mode, handle the previous timer overflowing.
    /// @param incrementCount Number of times to increment internal counter.
    void CascadeModeIncrement(int incrementCount);

    /// @brief Check if this timer is currently enabled.
    /// @return True if this timer is running.
    bool Running() const { return timerControl_.flags_.start_; }

    /// @brief Check if this timer is in cascade mode.
    /// @return True if timer in is cascade mode, false if it's in normal mode.
    bool CascadeMode() const { return (timerIndex_ != 0) && timerControl_.flags_.countUpTiming_; }

    /// @brief Whether an IRQ should be generated when this timer overflows.
    /// @return True if an IRQ should be generated.
    bool GenerateIRQ() const { return timerControl_.flags_.irqEnable_; }

private:
    /// @brief Update the internal counter and cancel the scheduled overflow event.
    void StopTimer();

    /// @brief Update the internal counter based on how many cycles have passed since this timer started.
    /// @param divider Clock divider.
    /// @param forceUpdate Optionally force an internal counter update regardless of counting mode.
    void UpdateInternalCounter(uint16_t divider, bool forceUpdate = false);

    /// @brief Get divider to use for calculating how often this timer increments.
    /// @return Clock divider.
    uint16_t GetDivider() const;

    union TIMXCNT
    {
        struct
        {
            uint32_t reload_ : 16;
            uint32_t prescalerSelection_ : 2;
            uint32_t countUpTiming_ : 1;
            uint32_t : 3;
            uint32_t irqEnable_ : 1;
            uint32_t start_ : 1;
            uint32_t : 8;
        } flags_;

        uint32_t word_;
    };

    std::array<uint8_t, 4> timerRegisters_;
    TIMXCNT& timerControl_;
    uint16_t internalTimer_;
    int timerIndex_;
    EventType overflowEvent_;
};
