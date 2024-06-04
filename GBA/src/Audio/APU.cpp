#include <Audio/APU.hpp>
#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <utility>
#include <Audio/Channel1.hpp>
#include <Audio/Channel2.hpp>
#include <Audio/Channel4.hpp>
#include <Audio/DmaAudio.hpp>
#include <Audio/Registers.hpp>
#include <System/EventScheduler.hpp>
#include <System/MemoryMap.hpp>
#include <Utilities/MemoryUtilities.hpp>

namespace Audio
{
APU::APU() :
    apuRegisters_(),
    soundcnt_l_(*reinterpret_cast<SOUNDCNT_L*>(&apuRegisters_[0x20])),
    soundcnt_h_(*reinterpret_cast<SOUNDCNT_H*>(&apuRegisters_[0x22])),
    soundcnt_x_(*reinterpret_cast<SOUNDCNT_X*>(&apuRegisters_[0x24])),
    soundbias_(*reinterpret_cast<SOUNDBIAS*>(&apuRegisters_[0x28])),
    dmaFifos_(soundcnt_h_)
{
    Scheduler.RegisterEvent(EventType::SampleAPU, std::bind(&Sample, this, std::placeholders::_1));
}

void APU::Reset()
{
    apuRegisters_.fill(0);
    channel1_.Reset();
    channel2_.Reset();
    channel4_.Reset();
    dmaFifos_.Reset();
    sampleBuffer_.Clear();

    Scheduler.ScheduleEvent(EventType::SampleAPU, CPU_CYCLES_PER_SAMPLE);
}

std::pair<uint32_t, bool> APU::ReadReg(uint32_t addr, AccessSize alignment)
{
    uint32_t value = 0;
    bool openBus = false;

    // Temporary register handling until all channels are implemented
    uint8_t* bytePtr = nullptr;

    switch (addr)
    {
        case CHANNEL_1_ADDR_MIN ... CHANNEL_1_ADDR_MAX:
            std::tie(value, openBus) = channel1_.ReadReg(addr, alignment);
            break;
        case CHANNEL_2_ADDR_MIN ... CHANNEL_2_ADDR_MAX:
            std::tie(value, openBus) = channel2_.ReadReg(addr, alignment);
            break;
        case CHANNEL_3_ADDR_MIN ... CHANNEL_3_ADDR_MAX:
        {
            size_t index = addr - SOUND_IO_ADDR_MIN;
            bytePtr = &apuRegisters_.at(index);
            break;
        }
        case CHANNEL_4_ADDR_MIN ... CHANNEL_4_ADDR_MAX:
            std::tie(value, openBus) = channel4_.ReadReg(addr, alignment);
            break;
        case APU_CONTROL_ADDR_MIN ... APU_CONTROL_ADDR_MAX:
            std::tie(value, openBus) = ReadApuCntReg(addr, alignment);
            break;
        case WAVE_RAM_ADDR_MIN ... WAVE_RAM_ADDR_MAX:
        {
            size_t index = addr - SOUND_IO_ADDR_MIN;
            bytePtr = &apuRegisters_.at(index);
            break;
        }
        case DMA_AUDIO_ADDR_MIN ... DMA_AUDIO_ADDR_MAX:
            std::tie(value, openBus) = dmaFifos_.ReadReg(addr, alignment);
            break;
        default:
            openBus = true;
            break;
    }

    // Temporary register handling until all channels are implemented
    if (bytePtr != nullptr)
    {
        value = ReadPointer(bytePtr, alignment);
    }

    return {value, openBus};
}

void APU::WriteReg(uint32_t addr, uint32_t value, AccessSize alignment)
{
    // Temporary register handling until all channels are implemented
    uint8_t* bytePtr = nullptr;

    switch (addr)
    {
        case CHANNEL_1_ADDR_MIN ... CHANNEL_1_ADDR_MAX:
        {
            bool triggered = channel1_.WriteReg(addr, value, alignment);

            if (triggered)
            {
                soundcnt_x_.chan1On = 1;
            }

            break;
        }
        case CHANNEL_2_ADDR_MIN ... CHANNEL_2_ADDR_MAX:
        {
            bool triggered = channel2_.WriteReg(addr, value, alignment);

            if (triggered)
            {
                soundcnt_x_.chan2On = 1;
            }

            break;
        }
        case CHANNEL_3_ADDR_MIN ... CHANNEL_3_ADDR_MAX:
        {
            size_t index = addr - SOUND_IO_ADDR_MIN;
            bytePtr = &apuRegisters_.at(index);
            break;
        }
        case CHANNEL_4_ADDR_MIN ... CHANNEL_4_ADDR_MAX:
        {
            bool triggered = channel4_.WriteReg(addr, value, alignment);

            if (triggered)
            {
                soundcnt_x_.chan4On = 1;
            }

            break;
        }
        case APU_CONTROL_ADDR_MIN ... APU_CONTROL_ADDR_MAX:
            WriteApuCntReg(addr, value, alignment);
            break;
        case WAVE_RAM_ADDR_MIN ... WAVE_RAM_ADDR_MAX:
        {
            size_t index = addr - SOUND_IO_ADDR_MIN;
            bytePtr = &apuRegisters_.at(index);
            break;
        }
        case DMA_AUDIO_ADDR_MIN ... DMA_AUDIO_ADDR_MAX:
            dmaFifos_.WriteReg(addr, value, alignment);
            break;
        default:
            break;
    }

    // Temporary register handling until all channels are implemented
    if (bytePtr != nullptr)
    {
        WritePointer(bytePtr, value, alignment);
    }
}

void APU::DrainBuffer(float* buffer)
{
    size_t index = 0;

    while (!sampleBuffer_.Empty())
    {
        auto [left, right] = sampleBuffer_.Pop();
        buffer[index++] = left;
        buffer[index++] = right;
    }
}

std::pair<uint32_t, bool> APU::ReadApuCntReg(uint32_t addr, AccessSize alignment)
{
    if ((0x0400'0084 <= addr) && (addr < 0x0400'0088))
    {
        if ((alignment == AccessSize::WORD) || (addr >= 0x0400'0086))
        {
            return {0, false};
        }
    }
    else if ((0x0400'0088 <= addr) && (addr < 0x0400'008C))
    {
        if ((alignment == AccessSize::WORD) || (addr >= 0x0400'008A))
        {
            return {0, false};
        }
    }
    else if (addr >= 0x0400'008C)
    {
        return {0, true};
    }

    if (channel1_.Expired())
    {
        soundcnt_x_.chan1On = 0;
    }

    if (channel2_.Expired())
    {
        soundcnt_x_.chan2On = 0;
    }

    if (channel4_.Expired())
    {
        soundcnt_x_.chan4On = 0;
    }

    size_t index = addr - SOUND_IO_ADDR_MIN;
    uint8_t* bytePtr = &apuRegisters_.at(index);
    uint32_t value = ReadPointer(bytePtr, alignment);
    return {value, false};
}

void APU::WriteApuCntReg(uint32_t addr, uint32_t value, AccessSize alignment)
{
    SOUNDCNT_X prevSoundcnt_x = soundcnt_x_;

    size_t index = addr - SOUND_IO_ADDR_MIN;
    uint8_t* bytePtr = &apuRegisters_.at(index);
    WritePointer(bytePtr, value, alignment);

    // Restore read only bits in SOUNDCNT_X
    soundcnt_x_.chan1On = prevSoundcnt_x.chan1On;
    soundcnt_x_.chan2On = prevSoundcnt_x.chan2On;
    soundcnt_x_.chan3On = prevSoundcnt_x.chan3On;
    soundcnt_x_.chan4On = prevSoundcnt_x.chan4On;

    // Reset FIFOs if requested and then reset request bit
    dmaFifos_.CheckFifoClear();
}

void APU::Sample(int extraCycles)
{
    Scheduler.ScheduleEvent(EventType::SampleAPU, CPU_CYCLES_PER_SAMPLE - extraCycles);

    if (sampleBuffer_.Full())
    {
        return;
    }

    int16_t leftSample = 0;
    int16_t rightSample = 0;

    if (soundcnt_x_.masterEnable)
    {
        // PSG channel samples
        int16_t psgLeftSample = 0;
        int16_t psgRightSample = 0;

        // Channel 1
        int16_t channel1Sample = channel1_.Sample();

        if (soundcnt_l_.chan1EnableLeft)
        {
            psgLeftSample += channel1Sample;
        }

        if (soundcnt_l_.chan1EnableRight)
        {
            psgRightSample += channel1Sample;
        }

        // Channel 2
        int16_t channel2Sample = channel2_.Sample();

        if (soundcnt_l_.chan2EnableLeft)
        {
            psgLeftSample += channel2Sample;
        }

        if (soundcnt_l_.chan2EnableRight)
        {
            psgRightSample += channel2Sample;
        }

        // Channel 4
        int16_t channel4Sample = channel4_.Sample();

        if (soundcnt_l_.chan4EnableLeft)
        {
            psgLeftSample += channel4Sample;
        }

        if (soundcnt_l_.chan4EnableRight)
        {
            psgRightSample += channel4Sample;
        }

        // Adjust PSG volume and mix in
        psgLeftSample = (psgLeftSample * 2) - 0x0F;
        psgRightSample = (psgRightSample * 2) - 0x0F;

        int psgMultiplier = 8;

        if (soundcnt_h_.psgVolume == 0)
        {
            psgMultiplier = 2;
        }
        else if (soundcnt_h_.psgVolume == 1)
        {
            psgMultiplier = 4;
        }

        psgLeftSample *= psgMultiplier;
        psgRightSample *= psgMultiplier;

        leftSample += psgLeftSample;
        rightSample += psgRightSample;

        // DMA Audio samples
        auto [fifoASample, fifoBSample] = dmaFifos_.Sample();

        if (soundcnt_h_.dmaEnableLeftA)
        {
            leftSample += fifoASample;
        }

        if (soundcnt_h_.dmaEnableRightA)
        {
            rightSample += fifoASample;
        }

        if (soundcnt_h_.dmaEnableLeftB)
        {
            leftSample += fifoBSample;
        }

        if (soundcnt_h_.dmaEnableRightB)
        {
            rightSample += fifoBSample;
        }

        // Apply SOUNDBIAS, clamp output, and convert to float
        leftSample += soundbias_.biasLevel;
        rightSample += soundbias_.biasLevel;

        std::clamp(leftSample, MIN_OUTPUT_LEVEL, MAX_OUTPUT_LEVEL);
        std::clamp(rightSample, MIN_OUTPUT_LEVEL, MAX_OUTPUT_LEVEL);
    }

    float leftOutput = (leftSample / 511.5) - 1.0;
    float rightOutput = (rightSample / 511.5) - 1.0;
    sampleBuffer_.Push({leftOutput, rightOutput});
}
}
