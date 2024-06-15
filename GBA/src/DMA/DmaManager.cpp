#include <DMA/DmaManager.hpp>
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <utility>
#include <DMA/DmaChannel.hpp>
#include <Logging/Logging.hpp>
#include <System/GameBoyAdvance.hpp>
#include <System/SystemControl.hpp>
#include <Utilities/MemoryUtilities.hpp>

DmaManager::DmaManager(GameBoyAdvance& gba) :
    gba_(gba),
    dmaChannels_({DmaChannel(0, InterruptType::DMA0, gba),
                  DmaChannel(1, InterruptType::DMA1, gba),
                  DmaChannel(2, InterruptType::DMA2, gba),
                  DmaChannel(3, InterruptType::DMA3, gba)})
{
    Scheduler.RegisterEvent(EventType::DmaComplete, std::bind(&EndDma, this, std::placeholders::_1));
}

void DmaManager::Reset()
{
    dmaActive_ = false;

    for (auto& dmaChannel : dmaChannels_)
    {
        dmaChannel.Reset();
    }

    vblank_.fill(false);
    hblank_.fill(false);
    fifoA_.fill(false);
    fifoB_.fill(false);
    videoCapture_.fill(false);
}

std::pair<uint32_t, bool> DmaManager::ReadReg(uint32_t addr, AccessSize alignment)
{
    if ((0x0400'00B0 <= addr) && (addr < 0x0400'00BC))
    {
        return dmaChannels_[0].ReadReg(addr, alignment);
    }
    else if ((0x0400'00BC <= addr) && (addr < 0x0400'00C8))
    {
        return dmaChannels_[1].ReadReg(addr, alignment);
    }
    else if ((0x0400'00C8 <= addr) && (addr < 0x0400'00D4))
    {
        return dmaChannels_[2].ReadReg(addr, alignment);
    }
    else if ((0x0400'00D4 <= addr) && (addr < 0x0400'00E0))
    {
        return dmaChannels_[3].ReadReg(addr, alignment);
    }

    return {0, true};
}

void DmaManager::WriteReg(uint32_t addr, uint32_t value, AccessSize alignment)
{
    int index = 0;
    auto xferType = DmaXfer::NO_CHANGE;

    if ((0x0400'00B0 <= addr) && (addr < 0x0400'00BC))
    {
        index = 0;
    }
    else if ((0x0400'00BC <= addr) && (addr < 0x0400'00C8))
    {
        index = 1;
    }
    else if ((0x0400'00C8 <= addr) && (addr < 0x0400'00D4))
    {
        index = 2;
    }
    else if ((0x0400'00D4 <= addr) && (addr < 0x0400'00E0))
    {
        index = 3;
    }

    xferType = dmaChannels_[index].WriteReg(addr, value, alignment);

    if (xferType != DmaXfer::NO_CHANGE)
    {
        vblank_[index] = false;
        hblank_[index] = false;
        fifoA_[index] = false;
        fifoB_[index] = false;
        videoCapture_[index] = false;

        switch (xferType)
        {
            case DmaXfer::NO_CHANGE:
            case DmaXfer::DISABLE:
            case DmaXfer::IMMEDIATE:
                break;
            case DmaXfer::VBLANK:
                vblank_[index] = true;
                break;
            case DmaXfer::HBLANK:
                hblank_[index] = true;
                break;
            case DmaXfer::FIFO_A:
                fifoA_[index] = true;
                break;
            case DmaXfer::FIFO_B:
                fifoB_[index] = true;
                break;
            case DmaXfer::VIDEO_CAPTURE:
                videoCapture_[index] = true;
                break;
        }

        if (xferType == DmaXfer::IMMEDIATE)
        {
            int dmaCycles = dmaChannels_[index].Execute();
            HandleDmaEvent(dmaCycles);
            dmaActive_ = true;
        }
    }
}

void DmaManager::CheckSpecialTiming(std::array<bool, 4>& enabledChannels, DmaXfer xferType)
{
    for (int i = 0; i < 4; ++i)
    {
        if (enabledChannels[i])
        {
            DmaChannel& channel = dmaChannels_[i];

            if (Logging::LogMgr.SystemLoggingEnabled())
            {
                Logging::LogMgr.LogDmaTransfer(i, xferType, channel.GetSrc(), channel.GetDest(), channel.GetCnt());
            }

            int dmaCycles = channel.Execute();
            enabledChannels[i] = channel.Enabled();

            if (dmaCycles > 0)
            {
                HandleDmaEvent(dmaCycles);
                dmaActive_ = true;
            }
        }
    }
}

void DmaManager::HandleDmaEvent(int cycles)
{
    auto currentDmaRemainingCycles = Scheduler.CyclesRemaining(EventType::DmaComplete);

    if (currentDmaRemainingCycles.has_value())
    {
        Scheduler.UnscheduleEvent(EventType::DmaComplete);
        cycles += currentDmaRemainingCycles.value();
    }

    Scheduler.ScheduleEvent(EventType::DmaComplete, cycles + 2);
}
