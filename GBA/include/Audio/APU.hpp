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
#include <Utilities/MemoryUtilities.hpp>
#include <Utilities/RingBuffer.hpp>

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

    // Producer thread functions

    /// @brief Only call from producer thread. Check number of free space for samples in internal buffer.
    /// @return Number of samples that can be buffered, with one sample being two left/right samples.
    size_t FreeBufferSpace() const;

    /// @brief Clear the current sample counter. Counter increments each time the APU is sampled.
    void ClearSampleCounter() { sampleCounter_ = 0; }

    /// @brief Check how many times the APU has been sampled since it was last cleared.
    /// @return Number of samples that have been produced.
    size_t GetSampleCounter() { return sampleCounter_; }

    // Consumer thread functions

    /// @brief Fill an external audio buffer with the requested number of samples.
    /// @param buffer Buffer to load internal audio buffer's samples into.
    /// @param cnt Number of samples to load into external buffer.
    void DrainBuffer(float* buffer, size_t cnt) { sampleBuffer_.Read(buffer, cnt); }

    /// @brief Check how many audio samples are currently saved in the internal buffer. One sample is a single left or right sample.
    /// @return Number of samples saved in internal buffer.
    size_t AvailableSamplesCount() const { return sampleBuffer_.GetAvailable(); }

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
    RingBuffer<float, BUFFER_SIZE> sampleBuffer_;
    size_t sampleCounter_;
};
}
