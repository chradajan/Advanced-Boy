#include <Audio/SoundController.hpp>
#include <algorithm>
#include <cstdint>
#include <functional>
#include <utility>
#include <Audio/Registers.hpp>
#include <CPU/CpuTypes.hpp>
#include <System/MemoryMap.hpp>
#include <System/EventScheduler.hpp>
#include <Utilities/CircularBuffer.hpp>
#include <Utilities/MemoryUtilities.hpp>

static constexpr bool UnusedSoundRegistersMap[80] = {
    0, 0, 0, 0,    0, 0, 1, 1,  // 0400'0060
    0, 0, 1, 1,    0, 0, 1, 1,  // 0400'0068
    0, 0, 0, 0,    0, 0, 1, 1,  // 0400'0070
    0, 0, 1, 1,    0, 0, 1, 1,  // 0400'0078
    0, 0, 0, 0,    0, 0, 1, 1,  // 0400'0080
    0, 0, 1, 1,    1, 1, 1, 1,  // 0400'0088
    0, 0, 0, 0,    0, 0, 0, 0,  // 0400'0090
    0, 0, 0, 0,    0, 0, 0, 0,  // 0400'0098
    0, 0, 0, 0,    0, 0, 0, 0,  // 0400'00A0
    1, 1, 1, 1,    1, 1, 1, 1   // 0400'00A8
};

namespace
{
ReadStatus AnalyzeRead(size_t index, AccessSize alignment)
{
    bool lsbUnused = UnusedSoundRegistersMap[index];

    if (lsbUnused)
    {
        return ReadStatus::OPEN_BUS;
    }

    for (size_t i = index + 1; i < index + static_cast<uint8_t>(alignment); ++i)
    {
        if (UnusedSoundRegistersMap[i])
        {
            return ReadStatus::ZERO;
        }
    }

    return ReadStatus::VALID;
}

float HPF(float input)
{
    static float capacitor = 0.0;
    float output = input - capacitor;
    capacitor = input - (output * 0.996);
    return output;
}
}

namespace Audio
{
SoundController::SoundController() :
    soundRegisters_(),
    soundControl_(*reinterpret_cast<SOUNDCNT*>(&soundRegisters_[32])),
    soundBias_(*reinterpret_cast<SOUNDBIAS*>(&soundRegisters_[40]))
{
    soundRegisters_.fill(0);
    Scheduler.RegisterEvent(EventType::SampleAPU, std::bind(&CollectSample, this, std::placeholders::_1), true);
    Scheduler.ScheduleEvent(EventType::SampleAPU, CPU_CYCLES_PER_SAMPLE);

    fifoASample_ = 0;
    fifoBSample_ = 0;
}

std::pair<uint32_t, bool> SoundController::ReadReg(uint32_t addr, AccessSize alignment)
{
    size_t index = addr - SOUND_IO_ADDR_MIN;
    ReadStatus status = AnalyzeRead(index, alignment);

    if (status == ReadStatus::OPEN_BUS)
    {
        return {0, true};
    }
    else if (status == ReadStatus::ZERO)
    {
        return {0, false};
    }

    uint8_t* bytePtr = &(soundRegisters_.at(index));
    uint32_t value = ReadPointer(bytePtr, alignment);
    return {value, false};
}

void SoundController::WriteReg(uint32_t addr, uint32_t value, AccessSize alignment)
{
    if (addr == 0x0400'0084)
    {
        value &= 0xFFFF'FFF0;
        value |= (soundControl_.chan1On_ | (soundControl_.chan2On_ << 1) | (soundControl_.chan3On_ << 2) | (soundControl_.chan4On_ << 3));
    }
    else if ((addr == FIFO_A_ADDR) || (addr == FIFO_B_ADDR))
    {
        WriteToFifo(addr, value);
        return;
    }

    size_t index = addr - SOUND_IO_ADDR_MIN;
    uint8_t* bytePtr = &(soundRegisters_.at(index));
    WritePointer(bytePtr, value, alignment);

    if (soundControl_.dmaSoundAResetFifo_)
    {
        soundControl_.dmaSoundAResetFifo_ = 0;
        fifoA_.Clear();
        fifoASample_ = 0;
    }

    if (soundControl_.dmaSoundBResetFifo_)
    {
        soundControl_.dmaSoundBResetFifo_ = 0;
        fifoB_.Clear();
        fifoBSample_ = 0;
    }
}

void SoundController::WriteToFifo(uint32_t addr, uint32_t value)
{
    DmaSoundFifo* fifo = nullptr;

    switch (addr)
    {
        case FIFO_A_ADDR:
            fifo = &fifoA_;
            break;
        case FIFO_B_ADDR:
            fifo = &fifoB_;
            break;
        default:
            break;
    }

    if (fifo == nullptr)
    {
        return;
    }

    for (int i = 0; i < 4; ++i)
    {
        if (fifo->Full())
        {
            break;
        }

        int8_t sample = value & MAX_U8;
        fifo->Push(sample);
        value >>= 8;
    }
}

std::pair<bool, bool> SoundController::UpdateDmaSound(int timer)
{
    bool replenishA = false;
    bool replenishB = false;

    if (soundControl_.dmaSoundATimerSelect_ == timer)
    {
        if (!fifoA_.Empty())
        {
            fifoASample_ = fifoA_.Pop();
        }

        replenishA = (fifoA_.Size() < 16);
    }

    if (soundControl_.dmaSoundBTimerSelect_ == timer)
    {
        if (!fifoB_.Empty())
        {
            fifoBSample_ = fifoB_.Pop();
        }

        replenishB = (fifoB_.Size() < 16);
    }

    return {replenishA, replenishB};
}

void SoundController::DrainInternalBuffer(float* externalBuffer)
{
    size_t index = 0;

    while (!internalBuffer_.Empty())
    {
        auto [left, right] = internalBuffer_.Pop();
        externalBuffer[index++] = left;
        externalBuffer[index++] = right;
    }
}

void SoundController::CollectSample(int extraCycles)
{
    Scheduler.ScheduleEvent(EventType::SampleAPU, CPU_CYCLES_PER_SAMPLE - extraCycles);

    if (InternalBufferFull())
    {
        return;
    }

    int16_t leftSample = 0;
    int16_t rightSample = 0;

    if (soundControl_.psgFifoMasterEnable_)
    {
        // Mix in FIFO A
        int16_t fifoA = soundControl_.dmaSoundAVolume_ ? fifoASample_ : (fifoASample_ / 2);

        if (soundControl_.dmaSoundAEnabledLeft_)
        {
            leftSample += fifoA;
        }

        if (soundControl_.dmaSoundAEnabledRight_)
        {
            rightSample += fifoA;
        }

        // Mix in FIFO B
        int16_t fifoB = soundControl_.dmaSoundBVolume_ ? fifoBSample_ : (fifoBSample_ / 2);

        if (soundControl_.dmaSoundBEnabledLeft_)
        {
            leftSample += fifoB;
        }

        if (soundControl_.dmaSoundBEnabledRight_)
        {
            rightSample += fifoB;
        }

        // Add SOUNDBIAS, clip to 10 bit output range, and convert to float
        leftSample += soundBias_.biasLevel_;
        rightSample += soundBias_.biasLevel_;

        leftSample = std::max(static_cast<int16_t>(0), leftSample);
        leftSample = std::min(static_cast<int16_t>(0x3FF), leftSample);
        rightSample = std::max(static_cast<int16_t>(0), rightSample);
        rightSample = std::min(static_cast<int16_t>(0x3FF), rightSample);
    }

    float leftOutput = HPF((leftSample / 511.5) - 1.0);
    float rightOutput = HPF((rightSample / 511.5) - 1.0);
    internalBuffer_.Push({leftOutput, rightOutput});
}
}
