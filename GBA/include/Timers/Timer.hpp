#pragma once

#include <array>
#include <cstdint>
#include <System/EventScheduler.hpp>
#include <System/SystemControl.hpp>
#include <Utilities/MemoryUtilities.hpp>

class Timer
{
public:
    /// @brief Initialize a timer.
    /// @param index Which timer (0-3) this is.
    /// @param overflowEvent Which event should fire when this timer overflows.
    /// @param interruptType What interrupt to request if IRQs are enabled for this timer.
    Timer(int index, EventType overflowEvent, InterruptType interruptType);

    /// @brief Reset a timer to its power-up state.
    void Reset();

    /// @brief Read a memory mapped timer register.
    /// @param addr Address of memory mapped register.
    /// @param alignment Number of bytes to read.
    /// @return Value of specified register.
    uint32_t ReadReg(uint32_t addr, AccessSize alignment);

    /// @brief Read a memory mapped timer register.
    /// @param addr Address of memory mapped register.
    /// @param value Value to write to register.
    /// @param alignment Number of bytes to write.
    void WriteReg(uint32_t addr, uint32_t value, AccessSize alignment);

    /// @brief Reload the internal counter and schedule a timer overflow event for a timer that was just enabled.
    void StartTimer();

    /// @brief Update a non-cascade mode timer that overflowed.
    /// @param extraCycles How many cycles have passed since this timer overflowed.
    /// @return Total number of overflow events that occurred.
    int Overflow(int extraCycles);

    /// @brief If this timer is in cascade mode, handle the previous timer overflowing.
    /// @param incrementCount Number of times to increment internal counter.
    void CascadeModeIncrement(int incrementCount);

    /// @brief Check if this timer is currently enabled.
    /// @return True if this timer is running.
    bool Running() const { return timerControl_.start; }

    /// @brief Check if this timer is in cascade mode.
    /// @return True if timer in is cascade mode, false if it's in normal mode.
    bool CascadeMode() const { return (timerIndex_ != 0) && timerControl_.countUpTiming; }

    /// @brief Whether an IRQ should be generated when this timer overflows.
    /// @return True if an IRQ should be generated.
    bool GenerateIRQ() const { return timerControl_.irqEnable; }

    /// @brief Get the interrupt type that this timer should request when it overflows and IRQs are enabled.
    /// @return This timer's associated interrupt.
    InterruptType GetInterruptType() const { return interruptType_; }

private:
    /// @brief Update the internal counter based on how many cycles have passed since this timer started.
    /// @param divider Clock divider to use for calculating internal counter value.
    void UpdateInternalCounter(uint16_t divider);

    /// @brief Get divider to use for calculating this timer's update frequency.
    /// @param prescalerSelection Frequency divider from control register.
    /// @return CPU clock divider.
    uint16_t GetDivider(uint16_t prescalerSelection) const;

    union TIMCNT
    {
        struct
        {
            uint16_t prescalerSelection : 2;
            uint16_t countUpTiming : 1;
            uint16_t : 3;
            uint16_t irqEnable : 1;
            uint16_t start : 1;
            uint16_t : 8;
        };

        uint16_t value;
    };

    // Registers
    std::array<uint8_t, 4> timerRegisters_;
    uint16_t& timerReload_;
    uint16_t internalTimer_;
    TIMCNT& timerControl_;

    // Timer info
    int const timerIndex_;
    EventType const overflowEvent_;
    InterruptType const interruptType_;
};
