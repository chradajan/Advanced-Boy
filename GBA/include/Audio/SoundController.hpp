#pragma once

#include <array>
#include <cstdint>
#include <utility>
#include <Audio/Registers.hpp>
#include <ARM7TDMI/CpuTypes.hpp>
#include <Utilities/CircularBuffer.hpp>
#include <Utilities/MemoryUtilities.hpp>

namespace Audio
{
constexpr int BUFFERED_SAMPLES = 2048;
constexpr int SAMPLING_FREQUENCY_HZ = 44100;
constexpr float SAMPLING_PERIOD = 1.0 / SAMPLING_FREQUENCY_HZ;
constexpr int CPU_CYCLES_PER_SAMPLE = (CPU::CPU_FREQUENCY_HZ / SAMPLING_FREQUENCY_HZ);

typedef CircularBuffer<int8_t, 32> DmaSoundFifo;

class SoundController
{
public:
    /// @brief Register and schedule the audio sampling event.
    SoundController();

    /// @brief Read a memory mapped audio register.
    /// @param addr Address of memory mapped register.
    /// @param alignment BYTE, HALFWORD, or WORD.
    /// @return Value of specified register and whether this read triggered open bus behavior.
    std::pair<uint32_t, bool> ReadReg(uint32_t addr, AccessSize alignment);

    /// @brief Write a memory mapped audio register.
    /// @param addr Address of memory mapped register.
    /// @param value Value to write to register.
    /// @param alignment BYTE, HALFWORD, or WORD.
    void WriteReg(uint32_t addr, uint32_t value, AccessSize alignment);

    /// @brief Push 4 samples into one of the DMA-Sound FIFOs.
    /// @param addr Must be FIFO_A_ADDR or FIFO_B_ADDR.
    /// @param value Samples to push.
    void WriteToFifo(uint32_t addr, uint32_t value);

    /// @brief Update the DMA-Sound channels.
    /// @param timer Which timer overflowed (0 or 1).
    /// @return Pair of bools indicating whether each FIFO is requesting to be replenished.
    std::pair<bool, bool> UpdateDmaSound(int timer);

    /// @brief Drain the internal buffer's samples into the specified buffer.
    /// @param externalBuffer Buffer to load internal buffer's samples into.
    void DrainInternalBuffer(float* externalBuffer);

    /// @brief Check if the internal buffer is holding its max number of samples.
    /// @return True if internal buffer is at max capacity.
    bool InternalBufferFull() const noexcept { return internalBuffer_.Full(); }

    /// @brief Check how many samples are currently loaded in internal buffer.
    /// @return Number of audio samples currently in buffer.
    int SampleCount() const noexcept { return internalBuffer_.Size(); }

private:
    /// @brief Callback function to sample the APU channels and generate an audio sample.
    /// @param extraCycles Number of cycles that passed since this event was supposed to execute.
    void CollectSample(int extraCycles);

    // Registers
    std::array<uint8_t, 0x50> soundRegisters_;
    SOUNDCNT& soundControl_;
    SOUNDBIAS& soundBias_;

    // DMA-Sound
    DmaSoundFifo fifoA_;
    DmaSoundFifo fifoB_;
    int8_t fifoASample_;
    int8_t fifoBSample_;

    // Internal sample buffer
    CircularBuffer<std::pair<float, float>, BUFFERED_SAMPLES> internalBuffer_;
};
}
