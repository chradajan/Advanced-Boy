#pragma once

#include <cstdint>
#include <utility>
#include <Audio/Registers.hpp>
#include <Utilities/CircularBuffer.hpp>
#include <Utilities/MemoryUtilities.hpp>

namespace Audio
{
class DmaAudio
{
    typedef CircularBuffer<int8_t, 32> DmaSoundFifo;

public:
    /// @brief Initialize DMA audio channels.
    /// @param soundcnt_h Reference to SOUNDCNT_H register.
    DmaAudio(SOUNDCNT_H& soundcnt_h);

    /// @brief Clear FIFOs.
    void Reset();

    /// @brief Read a DMA audio register. Always returns open bus status.
    /// @param addr Address of register to read.
    /// @param alignment Number of bytes to read.
    /// @return Always returns 0 and open bus status.
    std::pair<uint32_t, bool> ReadReg(uint32_t addr, AccessSize alignment);

    /// @brief Push samples to a DMA audio FIFO.
    /// @param addr Address of FIFO.
    /// @param value Sample value to push to FIFO.
    /// @param alignment Number of samples to push.
    void WriteReg(uint32_t addr, uint32_t value, AccessSize alignment);

    /// @brief Pop a sample off of FIFOs connected to the timer that overflowed.
    /// @param timerIndex Index of timer that overflowed.
    /// @return Whether FIFO A and FIFO B should trigger a DMA to refill them.
    std::pair<bool, bool> TimerOverflow(int timerIndex);

    /// @brief Sample the current output of each FIFO.
    /// @return Pair of signed FIFO samples.
    std::pair<int16_t, int16_t> Sample() const;

    /// @brief Check if either FIFO reset bits were written. If so, clear reset bits and FIFO.
    void CheckFifoClear();

private:
    /// @brief Push samples into a FIFO.
    /// @param fifo Reference to which FIFO to push samples to.
    /// @param value Value holding samples.
    /// @param sampleCount Number of samples to push.
    void FifoPush(DmaSoundFifo& fifo, uint32_t value, size_t sampleCount);

    SOUNDCNT_H& soundcnt_h_;

    DmaSoundFifo fifoA_;
    DmaSoundFifo fifoB_;

    int8_t fifoASample_;
    int8_t fifoBSample_;
};
}
