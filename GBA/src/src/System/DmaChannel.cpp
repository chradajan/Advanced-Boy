#include <System/DmaChannel.hpp>
#include <array>
#include <cstdint>
#include <tuple>
#include <System/GameBoyAdvance.hpp>
#include <System/InterruptManager.hpp>
#include <System/MemoryMap.hpp>
#include <System/Scheduler.hpp>
#include <System/Utilities.hpp>

DmaChannel::DmaChannel(int index) :
    dmaChannelRegisters_(),
    SAD_(*reinterpret_cast<uint32_t*>(&dmaChannelRegisters_[0])),
    DAD_(*reinterpret_cast<uint32_t*>(&dmaChannelRegisters_[4])),
    dmaControl_(*reinterpret_cast<DMAXCNT*>(&dmaChannelRegisters_[8])),
    dmaChannelIndex_(index)
{
    switch (index)
    {
        case 0:
            dmaEvent_ = EventType::DMA0;
            dmaInterrupt_ = InterruptType::DMA0;
            break;
        case 1:
            dmaEvent_ = EventType::DMA1;
            dmaInterrupt_ = InterruptType::DMA1;
            break;
        case 2:
            dmaEvent_ = EventType::DMA2;
            dmaInterrupt_ = InterruptType::DMA2;
            break;
        case 3:
            dmaEvent_ = EventType::DMA3;
            dmaInterrupt_ = InterruptType::DMA3;
            break;
    }
}

std::tuple<uint32_t, int, bool> DmaChannel::ReadRegister(uint32_t addr, AccessSize alignment)
{
    size_t index = addr % 12;

    if (index < 8)
    {
        return {0, 1, true};
    }

    uint8_t* bytePtr = &dmaChannelRegisters_[index];
    uint32_t value = ReadPointer(bytePtr, alignment);
    return {value, 1, false};
}

int DmaChannel::WriteRegister(uint32_t addr, uint32_t value, AccessSize alignment, GameBoyAdvance* gbaPtr)
{
    if (gbaPtr->activeDmaChannel_.value_or(4) == dmaChannelIndex_)
    {
        // Ignore edge case where channel writes to its own registers.
        return 1;
    }

    bool wasEnabled = dmaControl_.flags_.dmaEnable_;
    uint8_t previousTiming = dmaControl_.flags_.startTiming_;

    size_t index = addr % 12;
    uint8_t* bytePtr = &dmaChannelRegisters_[index];
    WritePointer(bytePtr, value, alignment);

    bool nowEnabled = dmaControl_.flags_.dmaEnable_;
    uint8_t currentTiming = dmaControl_.flags_.startTiming_;

    if (!wasEnabled && nowEnabled)
    {
        internalSrcAddr_ = SAD_ & (dmaChannelIndex_ == 0 ? 0x07FF'FFFF : 0x0FFF'FFFF);
        internalDestAddr_ = DAD_ & (dmaChannelIndex_ == 3 ? 0x0FFF'FFFF : 0x07FF'FFFF);
        xferAlignment_ = dmaControl_.flags_.xferType_ ? AccessSize::WORD : AccessSize::HALFWORD;
        SetInternalWordCount();
        ScheduleTransfer(gbaPtr);
    }
    else if (wasEnabled && !nowEnabled)
    {
        gbaPtr->dmaImmediately_[dmaChannelIndex_] = false;
        gbaPtr->dmaOnVBlank_[dmaChannelIndex_] = false;
        gbaPtr->dmaOnHBlank_[dmaChannelIndex_] = false;
    }
    else if (nowEnabled && (previousTiming != currentTiming))
    {
        gbaPtr->dmaImmediately_[dmaChannelIndex_] = false;
        gbaPtr->dmaOnVBlank_[dmaChannelIndex_] = false;
        gbaPtr->dmaOnHBlank_[dmaChannelIndex_] = false;

        switch (currentTiming)
        {
            case 1:  // VBlank
                gbaPtr->dmaOnVBlank_[dmaChannelIndex_] = true;
                break;
            case 2:  // HBlank
                gbaPtr->dmaOnHBlank_[dmaChannelIndex_] = true;
                break;
            case 3:  // Special
                break;
        }
    }

    return 1;
}

void DmaChannel::Execute(GameBoyAdvance* gbaPtr)
{
    while (internalWordCount_ > 0)
    {
        auto [value, readCycles] = gbaPtr->ReadMemory(internalSrcAddr_, xferAlignment_);
        gbaPtr->WriteMemory(internalDestAddr_, value, xferAlignment_);
        --internalWordCount_;
        UpdateInternalAddresses();
    }

    if (dmaControl_.flags_.repeat_ && (dmaControl_.flags_.startTiming_ != 0))
    {
        SetInternalWordCount();

        if (dmaControl_.flags_.destAddrControl_ == 3)
        {
            internalDestAddr_ = DAD_ & (dmaChannelIndex_ == 3 ? 0x0FFF'FFFF : 0x07FF'FFFF);
        }
    }
    else if (!dmaControl_.flags_.repeat_ || (dmaControl_.flags_.startTiming_ == 0))
    {
        dmaControl_.flags_.dmaEnable_ = 0;
        gbaPtr->dmaImmediately_[dmaChannelIndex_] = false;
        gbaPtr->dmaOnVBlank_[dmaChannelIndex_] = false;
        gbaPtr->dmaOnHBlank_[dmaChannelIndex_] = false;
    }

    if (dmaControl_.flags_.irqEnable_)
    {
        InterruptMgr.RequestInterrupt(dmaInterrupt_);
    }
}

void DmaChannel::PartiallyExecute(GameBoyAdvance* gbaPtr, int cycles)
{
    int wordsToTransfer = cycles / cyclesPerTransfer_;

    while ((wordsToTransfer > 0) && (internalWordCount_ > 0))
    {
        auto [value, readCycles] = gbaPtr->ReadMemory(internalSrcAddr_, xferAlignment_);
        gbaPtr->WriteMemory(internalDestAddr_, value, xferAlignment_);
        --wordsToTransfer;
        --internalWordCount_;
        UpdateInternalAddresses();
    }
}

int DmaChannel::TransferCycleCount(GameBoyAdvance* gbaPtr)
{
    int readCycles = 1;
    int writeCycles = 1;
    int processingTime = 2;
    bool romToRomDma = false;

    uint8_t const readPage = (internalSrcAddr_ & 0x0F00'0000) >> 24;
    uint8_t const writePage = (internalDestAddr_ & 0x0F00'0000) >> 24;

    if ((internalSrcAddr_ < 0x0800'0000) || (internalSrcAddr_ > 0x0FFF'FFFF))
    {
        // Either internal memory or open bus
        int cyclesPerRead = 1;

        switch (readPage)
        {
            case 0x02:  // 256K WRAM
                cyclesPerRead = (xferAlignment_ == AccessSize::WORD) ? 6 : 3;
                break;
            case 0x05:  // PRAM
                cyclesPerRead = (xferAlignment_ == AccessSize::WORD) ? 2 : 1;
                break;
            case 0x06:  // VRAM
                cyclesPerRead = (xferAlignment_ == AccessSize::WORD) ? 2 : 1;
                break;
            default:
                break;
        }

        readCycles = internalWordCount_ * cyclesPerRead;
    }
    else
    {
        // GamePak
        romToRomDma = true;
        int sequentialReads = ((xferAlignment_ == AccessSize::WORD) ? (2 * internalWordCount_) : internalWordCount_) - 1;
        readCycles = gbaPtr->gamePak_->NonSequentialAccessTime() +
                     (sequentialReads * gbaPtr->gamePak_->SequentialAccessTime(internalSrcAddr_));
    }

    if ((internalDestAddr_ < 0x0800'0000) || (internalDestAddr_ > 0x0FFF'FFFF))
    {
        // Either internal memory or open bus
        int cyclesPerWrite = 1;
        romToRomDma = false;

        switch (writePage)
        {
            case 0x02:  // 256K WRAM
                cyclesPerWrite = (xferAlignment_ == AccessSize::WORD) ? 6 : 3;
                break;
            case 0x05:  // PRAM
                cyclesPerWrite = (xferAlignment_ == AccessSize::WORD) ? 2 : 1;
                break;
            case 0x06:  // VRAM
                cyclesPerWrite = (xferAlignment_ == AccessSize::WORD) ? 2 : 1;
                break;
            default:
                break;
        }

        writeCycles = internalWordCount_ * cyclesPerWrite;
    }
    else
    {
        // GamePak
        writeCycles = internalWordCount_;
    }

    if (romToRomDma)
    {
        processingTime += 2;
    }

    totalTransferCycles_ = (readCycles + writeCycles) + processingTime;
    cyclesPerTransfer_ = totalTransferCycles_ / internalWordCount_;
    return totalTransferCycles_;
}

void DmaChannel::ScheduleTransfer(GameBoyAdvance* gbaPtr)
{
    switch (dmaControl_.flags_.startTiming_)
    {
        case 0:  // Immediately
            gbaPtr->dmaImmediately_[dmaChannelIndex_] = true;
            gbaPtr->ScheduleDMA(gbaPtr->dmaImmediately_);
            break;
        case 1:  // VBlank
            gbaPtr->dmaOnVBlank_[dmaChannelIndex_] = true;
            break;
        case 2:  // HBlank
            gbaPtr->dmaOnHBlank_[dmaChannelIndex_] = true;
            break;
        case 3:  // Special
            break;
    }
}

void DmaChannel::SetInternalWordCount()
{
    internalWordCount_ = dmaControl_.flags_.wordCount_ & (dmaChannelIndex_ == 3 ? 0xFFFF : 0x3FFF);

    if (internalWordCount_ == 0)
    {
        internalWordCount_ = (dmaChannelIndex_ == 3 ? 0x0001'0000 : 0x4000);
    }

}

void DmaChannel::UpdateInternalAddresses()
{
    switch (dmaControl_.flags_.destAddrControl_)
    {
        case 0:  // Increment
        case 3:  // Increment/Reload
            internalDestAddr_ += static_cast<uint8_t>(xferAlignment_);
            break;
        case 1:  // Decrement
            internalDestAddr_ -= static_cast<uint8_t>(xferAlignment_);
            break;
        case 2:  // Fixed
            break;
    }

    switch (dmaControl_.flags_.srcAddrControl_)
    {
        case 0:  // Increment
            internalSrcAddr_ += static_cast<uint8_t>(xferAlignment_);
            break;
        case 1:  // Decrement
            internalSrcAddr_ -= static_cast<uint8_t>(xferAlignment_);
            break;
        case 2:  // Fixed
        case 3:  // Forbidden
            break;
    }
}
