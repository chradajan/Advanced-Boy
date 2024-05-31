#pragma once

#include <array>
#include <cstdint>
#include <utility>
#include <Audio/Registers.hpp>
#include <Utilities/MemoryUtilities.hpp>

namespace Audio
{
class Channel2
{
public:
    /// @brief Initialize Channel 1 registers and event handling.
    Channel2();

    /// @brief Reset Channel 2 to its power-up state.
    void Reset();

    /// @brief Read a Channel 2 register.
    /// @param addr Address of register to read.
    /// @param alignment Number of bytes to read.
    /// @return Value of register and whether this read triggered open bus behavior.
    std::pair<uint32_t, bool> ReadReg(uint32_t addr, AccessSize alignment);

    /// @brief Write a Channel 2 register.
    /// @param addr Address of register to write.
    /// @param value Value to write to register.
    /// @param alignment Number of bytes to write.
    /// @return Whether this write triggered this channel to start/restart.
    bool WriteReg(uint32_t addr, uint32_t value, AccessSize alignment);

    /// @brief Sample Channel 2's current output.
    /// @return Channel 2 output value.
    int16_t Sample();

    /// @brief Check if Channel 2 has turned off due to its length timer expiring.
    /// @return True if length timer has expired.
    bool Expired() const { return lengthTimerExpired_; }

private:
    /// @brief Start Channel 2 processing.
    void Start();

    /// @brief Callback function to advance Channel 2's duty cycle step.
    /// @param extraCycles Number of cycles since this callback was supposed to execute.
    void Clock(int extraCycles);

    /// @brief Callback function to advance Channel 2's envelope.
    /// @param extraCycles Number of cycles since this callback was supposed to execute.
    void Envelope(int extraCycles);

    /// @brief Callback function to disable Channel 2 if its length timer was enabled when it was triggered.
    void LengthTimer(int) { lengthTimerExpired_ = true; };

    // Registers
    std::array<uint8_t, 8> channel2Registers_;
    SOUND2CNT_L& sound2cnt_l_;  // NR21, NR22 (Duty cycle, length, envelope)
    SOUND1CNT_X& sound2cnt_h_;  // NR23, NR24  (Period, control)

    // Latched values
    bool envelopeIncrease_;
    uint8_t envelopePace_;

    // State
    uint8_t currentVolume_;
    size_t dutyCycleIndex_;
    bool lengthTimerExpired_;
};
}
