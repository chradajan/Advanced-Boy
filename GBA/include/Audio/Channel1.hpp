#pragma once

#include <array>
#include <cstdint>
#include <utility>
#include <Audio/Registers.hpp>
#include <Utilities/MemoryUtilities.hpp>

namespace Audio
{
class Channel1
{
public:
    /// @brief Initialize Channel 1 registers and event handling.
    Channel1();

    /// @brief Reset Channel 1 to its power-up state.
    void Reset();

    /// @brief Read a Channel 1 register.
    /// @param addr Address of register to read.
    /// @param alignment Number of bytes to read.
    /// @return Value of register and whether this read triggered open bus behavior.
    std::pair<uint32_t, bool> ReadReg(uint32_t addr, AccessSize alignment);

    /// @brief Write a Channel 1 register.
    /// @param addr Address of register to write.
    /// @param value Value to write to register.
    /// @param alignment Number of bytes to write.
    /// @return Whether this write triggered this channel to start/restart.
    bool WriteReg(uint32_t addr, uint32_t value, AccessSize alignment);

    /// @brief Sample Channel 1's current output.
    /// @return Channel 1 output value.
    int16_t Sample();

    /// @brief Check if Channel 1 has turned off due to its length timer expiring.
    /// @return True if length timer has expired.
    bool Expired() const { return lengthTimerExpired_; }

private:
    /// @brief Start Channel 1 processing.
    void Start();

    /// @brief Callback function to advance Channel 1's duty cycle step.
    /// @param extraCycles Number of cycles since this callback was supposed to execute.
    void Clock(int extraCycles);

    /// @brief Callback function to advance Channel 1's envelope.
    /// @param extraCycles Number of cycles since this callback was supposed to execute.
    void Envelope(int extraCycles);

    /// @brief Callback function to disable Channel 1 if its length timer was enabled when it was triggered.
    void LengthTimer(int) { lengthTimerExpired_ = true; };

    /// @brief Callback function to advance Channel 1's frequency sweep.
    /// @param extraCycles Number of cycles since this callback was supposed to execute.
    void FrequencySweep(int extraCycles);

    // Registers
    std::array<uint8_t, 8> channel1Registers_;
    SOUND1CNT_L& sound1cnt_l_;  // NR10 (Frequency sweep)
    SOUND1CNT_H& sound1cnt_h_;  // NR11, NR12 (Duty cycle, length, envelope)
    SOUND1CNT_X& sound1cnt_x_;  // NR13, NR14  (Period, control)

    // Latched values
    bool envelopeIncrease_;
    uint8_t envelopePace_;

    // State
    uint8_t currentVolume_;
    size_t dutyCycleIndex_;
    bool lengthTimerExpired_;
    bool frequencyOverflow_;
};
}
