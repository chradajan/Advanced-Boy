#include <Audio/SoundController.hpp>
#include <cstdint>
#include <functional>
#include <utility>
#include <Audio/Registers.hpp>
#include <ARM7TDMI/CpuTypes.hpp>
#include <System/MemoryMap.hpp>
#include <System/Scheduler.hpp>
#include <Utilities/Utilities.hpp>
#include <Utilities/CircularBuffer.hpp>

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
}

namespace Audio
{
SoundController::SoundController() :
    soundRegisters_(),
    soundControl_(*reinterpret_cast<SOUNDCNT*>(&soundRegisters_[32])),
    soundBias_(*reinterpret_cast<SOUNDBIAS*>(&soundRegisters_[40]))
{
    soundRegisters_.fill(0);
    Scheduler.RegisterEvent(EventType::SampleAPU, std::bind(&CollectSample, this, std::placeholders::_1));
    Scheduler.ScheduleEvent(EventType::SampleAPU, CPU_CYCLES_PER_SAMPLE);
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

    size_t index = addr - SOUND_IO_ADDR_MIN;
    uint8_t* bytePtr = &(soundRegisters_.at(index));
    WritePointer(bytePtr, value, alignment);
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

    float leftSample = 0.0;
    float rightSample = 0.0;

    internalBuffer_.Push({leftSample, rightSample});
}
}
