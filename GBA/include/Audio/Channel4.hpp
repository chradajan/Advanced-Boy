#pragma once

#include <array>
#include <cstdint>
#include <utility>
#include <Audio/Registers.hpp>
#include <Utilities/MemoryUtilities.hpp>

namespace Audio
{
class Channel4
{
public:
     /// @brief Initialize Channel 4 registers and event handling.
    Channel4();

    /// @brief Reset Channel 4 to its power-up state.
    void Reset();

    /// @brief Read a Channel 4 register.
    /// @param addr Address of register to read.
    /// @param alignment Number of bytes to read.
    /// @return Value of register and whether this read triggered open bus behavior.
    std::pair<uint32_t, bool> ReadReg(uint32_t addr, AccessSize alignment);

    /// @brief Write a Channel 4 register.
    /// @param addr Address of register to write.
    /// @param value Value to write to register.
    /// @param alignment Number of bytes to write.
    /// @return Whether this write triggered this channel to start/restart.
    bool WriteReg(uint32_t addr, uint32_t value, AccessSize alignment);

    /// @brief Sample Channel 4's current output.
    /// @return Channel 4 output value.
    uint8_t Sample();

    /// @brief Check if Channel 4 has turned off due to its length timer expiring.
    /// @return True if length timer has expired.
    bool Expired() const { return lengthTimerExpired_; }

private:
    /// @brief Start Channel 4 processing.
    void Start();

    /// @brief Callback function to advance Channel 4's duty cycle step.
    /// @param extraCycles Number of cycles since this callback was supposed to execute.
    void Clock(int extraCycles);

    /// @brief Callback function to advance Channel 4's envelope.
    /// @param extraCycles Number of cycles since this callback was supposed to execute.
    void Envelope(int extraCycles);

    /// @brief Callback function to disable Channel 4 if its length timer was enabled when it was triggered.
    void LengthTimer(int) { lengthTimerExpired_ = true; };

    /// @brief Calculate how many CPU cycles until Channel 4 should be clocked again based on its frequency.
    /// @return Number of cycles between clocks.
    int EventCycles() const;

    // Registers
    std::array<uint8_t, 8> channel4Registers_;
    SOUND4CNT_L& sound4cnt_l_;  // NR41, NR42
    SOUND4CNT_H& sound4cnt_h_;  // NR43, NR44

    // Latched values
    bool envelopeIncrease_;
    uint8_t envelopePace_;

    // State
    uint8_t currentVolume_;
    bool lengthTimerExpired_;

    // Shift register
    uint16_t lsfr_;
};
}
