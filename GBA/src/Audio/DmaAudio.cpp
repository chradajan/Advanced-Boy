#include <Audio/DmaAudio.hpp>
#include <cstdint>
#include <utility>
#include <Audio/Registers.hpp>
#include <System/MemoryMap.hpp>
#include <Utilities/CircularBuffer.hpp>
#include <Utilities/MemoryUtilities.hpp>

namespace Audio
{
DmaAudio::DmaAudio(SOUNDCNT_H& soundcnt_h) :
    soundcnt_h_(soundcnt_h)
{
}

void DmaAudio::Reset()
{
    fifoA_.Clear();
    fifoB_.Clear();

    fifoASample_ = 0;
    fifoBSample_ = 0;
}

std::pair<uint32_t, bool> DmaAudio::ReadReg(uint32_t addr, AccessSize alignment)
{
    (void)addr;
    (void)alignment;
    return {0, true};
}

void DmaAudio::WriteReg(uint32_t addr, uint32_t value, AccessSize alignment)
{
    size_t sampleCount = static_cast<size_t>(alignment);

    if ((FIFO_A_ADDR <= addr) && (addr < FIFO_B_ADDR))
    {
        FifoPush(fifoA_, value, sampleCount);
    }
    else if ((FIFO_B_ADDR <= addr) && (addr < (FIFO_B_ADDR + 4)))
    {
        FifoPush(fifoB_, value, sampleCount);
    }
}

std::pair<bool, bool> DmaAudio::TimerOverflow(int timerIndex)
{
    bool replenishA = false;
    bool replenishB = false;

    if (soundcnt_h_.dmaTimerSelectA == timerIndex)
    {
        if (!fifoA_.Empty())
        {
            fifoASample_ = fifoA_.Pop();
        }

        replenishA = (fifoA_.Size() < 17);
    }

    if (soundcnt_h_.dmaTimerSelectB == timerIndex)
    {
        if (!fifoB_.Empty())
        {
            fifoBSample_ = fifoB_.Pop();
        }

        replenishB = (fifoB_.Size() < 17);
    }

    return {replenishA, replenishB};
}

std::pair<int16_t, int16_t> DmaAudio::Sample() const
{
    int16_t sampleA = (soundcnt_h_.dmaVolumeA ? (fifoASample_ * 4) : (fifoASample_ * 2));
    int16_t sampleB = (soundcnt_h_.dmaVolumeB ? (fifoBSample_ * 4) : (fifoBSample_ * 2));
    return {sampleA, sampleB};
}

void DmaAudio::CheckFifoClear()
{
    if (soundcnt_h_.dmaResetA)
    {
        fifoA_.Clear();
        fifoASample_ = 0;
        soundcnt_h_.dmaResetA = 0;
    }

    if (soundcnt_h_.dmaResetB)
    {
        fifoB_.Clear();
        fifoBSample_ = 0;
        soundcnt_h_.dmaResetB = 0;
    }
}

void DmaAudio::FifoPush(DmaSoundFifo& fifo, uint32_t value, size_t sampleCount)
{
    while ((sampleCount > 0) && !fifo.Full())
    {
        int8_t sample = value & MAX_U8;
        fifo.Push(sample);
        value >>= 8;
        --sampleCount;
    }
}
}
