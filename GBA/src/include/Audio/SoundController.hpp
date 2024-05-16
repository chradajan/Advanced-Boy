#pragma once

#include <cstdint>
#include <ARM7TDMI/CpuTypes.hpp>
#include <Utilities/CircularBuffer.hpp>

namespace Audio
{
constexpr int BUFFERED_SAMPLES = 2048;
constexpr int SAMPLING_FREQUENCY_HZ = 44100;
constexpr float SAMPLING_PERIOD = 1.0 / SAMPLING_FREQUENCY_HZ;
constexpr int CPU_CYCLES_PER_SAMPLE = (CPU::CPU_FREQUENCY_HZ / SAMPLING_FREQUENCY_HZ);

class SoundController
{
public:
    /// @brief Register and schedule the audio sampling event.
    SoundController();

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

    CircularBuffer<std::pair<float, float>, BUFFERED_SAMPLES> internalBuffer_;
};
}
