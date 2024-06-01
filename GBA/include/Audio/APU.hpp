#pragma once

#include <array>
#include <cstdint>
#include <utility>
#include <Audio/Channel1.hpp>
#include <Audio/Channel2.hpp>
#include <Audio/Channel4.hpp>
#include <Audio/Constants.hpp>
#include <Audio/DmaAudio.hpp>
#include <Audio/Registers.hpp>
#include <CPU/CpuTypes.hpp>
#include <Utilities/CircularBuffer.hpp>
#include <Utilities/MemoryUtilities.hpp>

namespace Audio
{
class APU
{
public:
    /// @brief Initialize APU registers and event registration.
    APU();

    /// @brief Reset APU to power-up state.
    void Reset();

    /// @brief Read an APU register.
    /// @param addr Address of register to read.
    /// @param alignment Number of bytes to read.
    /// @return Value at specified register and whether this read triggered open bus behavior.
    std::pair<uint32_t, bool> ReadReg(uint32_t addr, AccessSize alignment);

    /// @brief Write an APU register.
    /// @param addr Address of register to write.
    /// @param value Value to write to register.
    /// @param alignment Number of bytes to write.
    void WriteReg(uint32_t addr, uint32_t value, AccessSize alignment);

    /// @brief Update DMA audio when timer overflows.
    /// @param timerIndex Which timer overflowed.
    /// @return Whether FIFO A and FIFO B should trigger a DMA to refill them.
    std::pair<bool, bool> TimerOverflow(int timerIndex) { return dmaFifos_.TimerOverflow(timerIndex); }

    /// @brief Check how many samples have been collected in the internal buffer.
    /// @return Number of samples in internal buffer.
    size_t SampleCount() const { return sampleBuffer_.Size(); }

    /// @brief Drain all samples in the internal buffer into an external playback buffer.
    /// @param buffer Pointer to buffer to drain internal samples into.
    void DrainBuffer(float* buffer);

private:
    /// @brief Read an APU control register.
    /// @param addr Address of register to read.
    /// @param alignment Number of bytes to read.
    /// @return Value at specified register and whether this read triggered open bus behavior.
    std::pair<uint32_t, bool> ReadApuCntReg(uint32_t addr, AccessSize alignment);

    /// @brief Write an APU control register.
    /// @param addr Address of register to write.
    /// @param value Value to write to register.
    /// @param alignment Number of bytes to write.
    void WriteApuCntReg(uint32_t addr, uint32_t value, AccessSize alignment);

    /// @brief Sample all APU channels and push a sample to the internal buffer.
    /// @param extraCycles Number of cycles since this callback was supposed to execute.
    void Sample(int extraCycles);

    // Registers
    std::array<uint8_t, 0x4C> apuRegisters_;
    SOUNDCNT_L& soundcnt_l_;
    SOUNDCNT_H& soundcnt_h_;
    SOUNDCNT_X& soundcnt_x_;
    SOUNDBIAS& soundbias_;

    // Channels
    Channel1 channel1_;
    Channel2 channel2_;
    Channel4 channel4_;
    DmaAudio dmaFifos_;

    // Internal sample buffer
    CircularBuffer<std::pair<float, float>, SAMPLE_BUFFER_SIZE> sampleBuffer_;
};
}
