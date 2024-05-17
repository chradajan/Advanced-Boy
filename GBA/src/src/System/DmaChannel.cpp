#include <System/DmaChannel.hpp>
#include <array>
#include <cstdint>
#include <utility>
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
    dmaChannelIndex_(index),
    eepromRead_(false),
    eepromWrite_(false)
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

std::pair<uint32_t, bool> DmaChannel::ReadReg(uint32_t addr, AccessSize alignment)
{
    size_t index = addr % 12;

    if ((index < 8) || (index == 9))
    {
        return {0, true};
    }
    else if (index == 8)
    {
        if (alignment == AccessSize::WORD)
        {
            return {0, false};
        }
        else
        {
            return {0, true};
        }
    }

    uint8_t* bytePtr = &dmaChannelRegisters_[index];
    uint32_t value = ReadPointer(bytePtr, alignment);
    return {value, false};
}

void DmaChannel::WriteReg(uint32_t addr, uint32_t value, AccessSize alignment, GameBoyAdvance* gbaPtr)
{
    if (gbaPtr->activeDmaChannel_.value_or(4) == dmaChannelIndex_)
    {
        // Ignore edge case where channel writes to its own registers.
        return;
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

        internalSrcAddr_ = AlignAddress(internalSrcAddr_, xferAlignment_);
        internalDestAddr_ = AlignAddress(internalDestAddr_, xferAlignment_);

        SetInternalWordCount();
        ScheduleTransfer(gbaPtr);
    }
    else if (wasEnabled && !nowEnabled)
    {
        gbaPtr->dmaImmediately_[dmaChannelIndex_] = false;
        gbaPtr->dmaOnVBlank_[dmaChannelIndex_] = false;
        gbaPtr->dmaOnHBlank_[dmaChannelIndex_] = false;
        gbaPtr->dmaSoundFifo_[dmaChannelIndex_] = false;
    }
    else if (nowEnabled && (previousTiming != currentTiming))
    {
        gbaPtr->dmaImmediately_[dmaChannelIndex_] = false;
        gbaPtr->dmaOnVBlank_[dmaChannelIndex_] = false;
        gbaPtr->dmaOnHBlank_[dmaChannelIndex_] = false;
        gbaPtr->dmaSoundFifo_[dmaChannelIndex_] = false;

        switch (currentTiming)
        {
            case 1:  // VBlank
                gbaPtr->dmaOnVBlank_[dmaChannelIndex_] = true;
                break;
            case 2:  // HBlank
                gbaPtr->dmaOnHBlank_[dmaChannelIndex_] = true;
                break;
            case 3:  // Special
            {
                if ((dmaChannelIndex_ == 1) || (dmaChannelIndex_ == 2))
                {
                    gbaPtr->dmaSoundFifo_[dmaChannelIndex_] = true;
                }

                break;
            }
        }
    }
}

void DmaChannel::Execute(GameBoyAdvance* gbaPtr)
{
    if (eepromRead_ && eepromWrite_)
    {
        // This probably won't happen, so just do nothing. TODO?
    }
    else if (eepromRead_)
    {
        // EEPROM can only be accessed by DMA channel 3, with src and dest addresses set to increment, and using 16 bit transfers.
        // Reads always transfer 64 bits with 4 throwaway bits, so check transfer length too.
        if ((dmaChannelIndex_ == 3) &&
            (dmaControl_.flags_.destAddrControl_ == 0) &&
            (dmaControl_.flags_.srcAddrControl_ == 0) &&
            (dmaControl_.flags_.xferType_ == 0) &&
            (internalWordCount_ == 68))
        {
            uint64_t eepromData = gbaPtr->gamePak_->GetEepromDoubleWord();

            for (int i = 0; i < 4; ++i)
            {
                // First send 4 bits of irrelevant data that should be ignored.
                gbaPtr->WriteMemory(internalDestAddr_, 0, AccessSize::HALFWORD);
                internalDestAddr_ += 2;
                internalSrcAddr_ += 2;
                --internalWordCount_;
            }

            while (internalWordCount_ > 0)
            {
                uint16_t value = (eepromData & 0x8000'0000'0000'0000) >> 63;
                eepromData <<= 1;
                gbaPtr->WriteMemory(internalDestAddr_, value, AccessSize::HALFWORD);
                internalDestAddr_ += 2;
                internalSrcAddr_ += 2;
                --internalWordCount_;
            }
        }
    }
    else if (eepromWrite_)
    {
        // See EEPROM restrictions above.
        if ((dmaChannelIndex_ == 3) &&
            (dmaControl_.flags_.destAddrControl_ == 0) &&
            (dmaControl_.flags_.srcAddrControl_ == 0) &&
            (dmaControl_.flags_.xferType_ == 0) &&
            ((internalWordCount_ == 9) || (internalWordCount_ == 17) || (internalWordCount_ == 73) || (internalWordCount_ == 81)))
        {
            if ((internalWordCount_ == 9) || (internalWordCount_ == 17))
            {
                // Write to set up for a EEPROM read.
                size_t index = 0;
                int indexLength = internalWordCount_ - 3;

                // Ignore first 2 bits (which should be 0b11 to indicate a read request)
                internalWordCount_ -= 2;
                internalDestAddr_ += 4;
                internalSrcAddr_ += 4;

                // Get EEPROM index to read from
                for (int i = 0; i < indexLength; ++i)
                {
                    index <<= 1;
                    index |= (gbaPtr->ReadMemory(internalSrcAddr_, AccessSize::HALFWORD).first & 0x01);
                    --internalWordCount_;
                    internalDestAddr_ += 2;
                    internalSrcAddr_ += 2;
                }

                // Ignore last bit (which should be 0)
                --internalWordCount_;
                internalDestAddr_ += 2;
                internalSrcAddr_ += 2;

                gbaPtr->gamePak_->SetEepromIndex(index, indexLength);
            }
            else
            {
                // Write double word to EEPROM
                size_t index = 0;
                int indexLength = internalWordCount_ - 67;
                uint64_t doubleWord = 0;

                // Ignore first 2 bits (which should be 0b10 to indicate a write request)
                internalWordCount_ -= 2;
                internalDestAddr_ += 4;
                internalSrcAddr_ += 4;

                // Get EEPROM index to write to
                for (int i = 0; i < indexLength; ++i)
                {
                    index <<= 1;
                    index |= (gbaPtr->ReadMemory(internalSrcAddr_, AccessSize::HALFWORD).first & 0x01);
                    --internalWordCount_;
                    internalDestAddr_ += 2;
                    internalSrcAddr_ += 2;
                }

                // Get double word from memory buffer to transfer to EEPROM
                for (int i = 0; i < 64; ++i)
                {
                    doubleWord <<= 1;
                    doubleWord |= (gbaPtr->ReadMemory(internalSrcAddr_, AccessSize::HALFWORD).first & 0x01);
                    --internalWordCount_;
                    internalDestAddr_ += 2;
                    internalSrcAddr_ += 2;
                }

                // Ignore last bit (which should be 0)
                --internalWordCount_;
                internalDestAddr_ += 2;
                internalSrcAddr_ += 2;

                gbaPtr->gamePak_->WriteToEeprom(index, indexLength, doubleWord);
            }
        }
    }
    else
    {
        // Normal DMA transfer
        while (internalWordCount_ > 0)
        {
            auto [value, readCycles] = gbaPtr->ReadMemory(internalSrcAddr_, xferAlignment_);
            gbaPtr->WriteMemory(internalDestAddr_, value, xferAlignment_);
            --internalWordCount_;
            UpdateInternalAddresses();
        }
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
    if (eepromRead_ || eepromWrite_)
    {
        return;
    }

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

    eepromRead_ = false;
    eepromWrite_ = false;

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
        readCycles = gbaPtr->gamePak_->AccessTime(internalSrcAddr_, false, xferAlignment_);
        readCycles += (gbaPtr->gamePak_->AccessTime(internalSrcAddr_, true, xferAlignment_) * (internalWordCount_ - 1));
        eepromRead_ = gbaPtr->gamePak_->EepromAccess(internalSrcAddr_);
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
        writeCycles = gbaPtr->gamePak_->AccessTime(internalDestAddr_, false, xferAlignment_);
        writeCycles += (gbaPtr->gamePak_->AccessTime(internalDestAddr_, true, xferAlignment_) * (internalWordCount_ - 1));
        eepromWrite_ = gbaPtr->gamePak_->EepromAccess(internalDestAddr_);
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
        {
            if ((dmaChannelIndex_ == 1) || (dmaChannelIndex_ == 2))
            {
                gbaPtr->dmaSoundFifo_[dmaChannelIndex_] = true;
            }
            break;
        }
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
