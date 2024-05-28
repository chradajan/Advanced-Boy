#include <DMA/DmaChannel.hpp>
#include <cstdint>
#include <utility>
#include <System/GameBoyAdvance.hpp>
#include <System/MemoryMap.hpp>
#include <System/SystemControl.hpp>
#include <Utilities/MemoryUtilities.hpp>

DmaChannel::DmaChannel(int index, InterruptType interrupt, GameBoyAdvance& gba) :
    dmaRegisters_(),
    sad_(*reinterpret_cast<uint32_t*>(&dmaRegisters_[0])),
    dad_(*reinterpret_cast<uint32_t*>(&dmaRegisters_[4])),
    wordCount_(*reinterpret_cast<uint16_t*>(&dmaRegisters_[8])),
    dmacnt_(*reinterpret_cast<DMACNT*>(&dmaRegisters_[10])),
    channelIndex_(index),
    interruptType_(interrupt),
    gba_(gba)
{
}

void DmaChannel::Reset()
{
    dmaRegisters_.fill(0);
    internalSrcAddr_ = 0;
    internalDestAddr_ = 0;
    internalWordCount_ = 0;
}

std::pair<uint32_t, bool> DmaChannel::ReadReg(uint32_t addr, AccessSize alignment)
{
    uint32_t value = 0;
    size_t index = (addr - DMA_TRANSFER_CHANNELS_IO_ADDR_MIN) % 12;

    if ((index == 8) && (alignment == AccessSize::WORD))
    {
        return {0, false};
    }
    else if (index < 10)
    {
        return {0, true};
    }

    uint8_t* bytePtr = &dmaRegisters_.at(index);
    value = ReadPointer(bytePtr, alignment);
    return {value, false};
}

DmaXfer DmaChannel::WriteReg(uint32_t addr, uint32_t value, AccessSize alignment)
{
    DMACNT prevCnt = dmacnt_;
    auto xferType = DmaXfer::NO_CHANGE;
    size_t index = (addr - DMA_TRANSFER_CHANNELS_IO_ADDR_MIN) % 12;
    uint8_t* bytePtr = &dmaRegisters_.at(index);
    WritePointer(bytePtr, value, alignment);

    if (!prevCnt.enable && dmacnt_.enable)  // Turning on a previously disabled channel
    {
        SetInternalRegisters();
        xferType = DetermineStartTiming();
    }
    else if (prevCnt.enable && !dmacnt_.enable)  // Turning off a previously enabled channel
    {
        xferType = DmaXfer::DISABLE;
    }
    else if (prevCnt.enable && dmacnt_.enable && (prevCnt.startTiming != dmacnt_.startTiming))  // Timing change while running
    {
        xferType = DetermineStartTiming();
    }

    return xferType;
}

int DmaChannel::Execute()
{
    bool eepromRead = gba_.gamePak_->EepromAccess(internalSrcAddr_);
    bool eepromWrite = gba_.gamePak_->EepromAccess(internalDestAddr_);
    bool fifoXfer = IsFifoXfer();
    int xferTime = 0;

    if (eepromRead || eepromWrite)
    {
        xferTime = ExecuteEepromXfer(eepromRead, eepromWrite);
    }
    else if (fifoXfer)
    {
        xferTime = ExecuteFifoXfer();
    }
    else
    {
        xferTime = ExecuteNormalXfer();
    }

    if (dmacnt_.repeat)
    {
        internalWordCount_ = wordCount_ & ((channelIndex_ == 3) ? 0xFFFF : 0x3FFF);

        if (internalWordCount_ == 0)
        {
            internalWordCount_ = (channelIndex_ == 3) ? 0x0001'0000 : 0x4000;
        }

        if (dmacnt_.destAddrCnt == 3)
        {
            internalDestAddr_ = dad_ & ((channelIndex_ == 3) ? 0x0FFF'FFFF : 0x07FF'FFFF);
        }
    }

    if (!dmacnt_.repeat || (dmacnt_.startTiming == 0))
    {
        dmacnt_.enable = 0;
    }

    if (dmacnt_.irq)
    {
        SystemController.RequestInterrupt(interruptType_);
    }

    return xferTime;
}

int DmaChannel::ExecuteNormalXfer()
{
    int xferCycles = 0;
    AccessSize alignment = dmacnt_.xferType ? AccessSize::WORD : AccessSize::HALFWORD;
    int8_t srcAddrDelta = 0;
    int8_t destAddrDelta = 0;

    switch (dmacnt_.srcAddrCnt)
    {
        case 0:  // Increment
            srcAddrDelta = static_cast<int8_t>(alignment);
            break;
        case 1:  // Decrement
            srcAddrDelta = -1 * static_cast<int8_t>(alignment);
            break;
        case 2:  // Fixed
        case 3:  // Prohibited
            break;
    }

    switch (dmacnt_.destAddrCnt)
    {
        case 0:  // Increment
            destAddrDelta = static_cast<int8_t>(alignment);
            break;
        case 1:  // Decrement
            destAddrDelta = -1 * static_cast<int8_t>(alignment);
            break;
        case 2:  // Fixed
            break;
        case 3:  // Increment/Reload
            destAddrDelta = static_cast<int8_t>(alignment);
            break;
    }

    while (internalWordCount_ > 0)
    {
        auto [value, readCycles] = gba_.ReadMemory(internalSrcAddr_, alignment);
        int writeCycles = gba_.WriteMemory(internalDestAddr_, value, alignment);
        xferCycles += readCycles + writeCycles;
        --internalWordCount_;
        internalSrcAddr_ += srcAddrDelta;
        internalDestAddr_ += destAddrDelta;
    }

    return xferCycles;
}

int DmaChannel::ExecuteEepromXfer(bool read, bool write)
{
    int xferCycles = 0;

    if (read && write)
    {
        return xferCycles;
    }

    if (read)
    {
        if ((channelIndex_ == 3) &&
            (dmacnt_.destAddrCnt == 0) && (dmacnt_.srcAddrCnt == 0) &&
            (dmacnt_.xferType == 0) &&
            (internalWordCount_ == 68))
        {
            auto [doubleWord, readCycles] = gba_.gamePak_->ReadFromEeprom();
            xferCycles += readCycles;

            for (int i = 0; i < 4; ++i)
            {
                xferCycles += gba_.WriteMemory(internalDestAddr_, 0, AccessSize::HALFWORD);
                internalDestAddr_ += static_cast<uint32_t>(AccessSize::HALFWORD);
                internalSrcAddr_ += static_cast<uint32_t>(AccessSize::HALFWORD);
                --internalWordCount_;
            }

            while (internalWordCount_ > 0)
            {
                uint16_t value = (doubleWord & MSB_64) >> 63;
                doubleWord <<= 1;
                xferCycles += gba_.WriteMemory(internalDestAddr_, value, AccessSize::HALFWORD);
                internalDestAddr_ += static_cast<uint32_t>(AccessSize::HALFWORD);
                internalSrcAddr_ += static_cast<uint32_t>(AccessSize::HALFWORD);
                --internalWordCount_;
            }
        }
    }
    else if (write)
    {
        if ((channelIndex_ == 3) &&
            (dmacnt_.destAddrCnt == 0) && (dmacnt_.srcAddrCnt == 0) &&
            (dmacnt_.xferType == 0) &&
            ((internalWordCount_ == 9) || (internalWordCount_ == 17) || (internalWordCount_ == 73) || (internalWordCount_ == 81)))
        {
            if ((internalWordCount_ == 9) || (internalWordCount_ == 17))  // Writing to set up for a read
            {
                uint16_t index = 0;
                int indexLength = internalWordCount_ - 3;

                for (int i = 0; i < 2; ++i)
                {
                    // Read and throw away first 2 bits (should be 0b11).
                    auto [value, readCycles] = ReadForEepromXfer();
                    xferCycles += readCycles;
                }

                for (int i = 0; i < indexLength; ++i)
                {
                    // Read index bitstream from memory.
                    index <<= 1;
                    auto [value, readCycles] = ReadForEepromXfer();
                    index |= (value & 0x01);
                    xferCycles += readCycles;
                }

                // Read final bit (should be 0)
                auto [value, readCycles] = ReadForEepromXfer();
                xferCycles += readCycles;

                // Send bitstream to EEPROM
                xferCycles += gba_.gamePak_->SetEepromIndex(index, indexLength);
            }
            else  // Writing a double word to EEPROM
            {
                uint16_t index = 0;
                int indexLength = internalWordCount_ - 67;
                uint64_t doubleWord = 0;

                for (int i = 0; i < 2; ++i)
                {
                    // Read and throw away first 2 bits (should be 0b10).
                    auto [value, readCycles] = ReadForEepromXfer();
                    xferCycles += readCycles;
                }

                for (int i = 0; i < indexLength; ++i)
                {
                    // Read index bitstream from memory.
                    index <<= 1;
                    auto [value, readCycles] = ReadForEepromXfer();
                    index |= (value & 0x01);
                    xferCycles += readCycles;
                }

                for (int i = 0; i < 64; ++i)
                {
                    // Get double word from memory to write to EEPROM.
                    doubleWord <<= 1;
                    auto [value, readCycles] = ReadForEepromXfer();
                    doubleWord |= (value & 0x01);
                    xferCycles += readCycles;
                }

                // Read final bit (should be 0).
                auto [value, readCycles] = ReadForEepromXfer();
                xferCycles += readCycles;

                // Send double word to EEPROM.
                xferCycles += gba_.gamePak_->WriteToEeprom(index, indexLength, doubleWord);
            }
        }
    }

    return xferCycles;
}

int DmaChannel::ExecuteFifoXfer()
{
    int xferCycles = 0;
    int8_t srcAddrDelta = 0;
    auto alignment = AccessSize::WORD;

    switch (dmacnt_.srcAddrCnt)
    {
        case 0:  // Increment
            srcAddrDelta = static_cast<int8_t>(alignment);
            break;
        case 1:  // Decrement
            srcAddrDelta = -1 * static_cast<int8_t>(alignment);
            break;
        case 2:  // Fixed
        case 3:  // Prohibited
            break;
    }

    for (int i = 0; i < 4; ++i)
    {
        auto [value, readCycles] = gba_.ReadMemory(internalSrcAddr_, alignment);
        gba_.apu_.WriteToFifo(internalDestAddr_, value);
        xferCycles += readCycles + 1;
        internalSrcAddr_ += srcAddrDelta;
    }

    return xferCycles;
}

bool DmaChannel::IsFifoXfer() const
{
    return dmacnt_.repeat && 
           ((internalDestAddr_ == FIFO_A_ADDR) || (internalDestAddr_ == FIFO_B_ADDR)) &&
           ((channelIndex_ == 1) || (channelIndex_ == 2));
}

DmaXfer DmaChannel::DetermineStartTiming() const
{
    auto xferType = DmaXfer::NO_CHANGE;

    switch (dmacnt_.startTiming)
    {
        case 0:
            xferType = DmaXfer::IMMEDIATE;
            break;
        case 1:
            xferType = DmaXfer::VBLANK;
            break;
        case 2:
            xferType = DmaXfer::HBLANK;
            break;
        case 3:
        {
            if ((channelIndex_ == 1) || (channelIndex_ == 2))
            {
                if (dad_ == FIFO_A_ADDR)
                {
                    xferType = DmaXfer::FIFO_A;
                }
                else if (dad_ == FIFO_B_ADDR)
                {
                    xferType = DmaXfer::FIFO_B;
                }
            }
            else if (channelIndex_ == 3)
            {
                xferType = DmaXfer::VIDEO_CAPTURE;
            }
        }
    }

    return xferType;
}

void DmaChannel::SetInternalRegisters()
{
    internalSrcAddr_ = sad_ & ((channelIndex_ == 0) ? 0x07FF'FFFF : 0x0FFF'FFFF);
    internalDestAddr_ = dad_ & ((channelIndex_ == 3) ? 0x0FFF'FFFF : 0x07FF'FFFF);
    internalWordCount_ = wordCount_ & ((channelIndex_ == 3) ? 0xFFFF : 0x3FFF);

    if (internalWordCount_ == 0)
    {
        internalWordCount_ = (channelIndex_ == 3) ? 0x0001'0000 : 0x4000;
    }
}

std::pair<uint32_t, int> DmaChannel::ReadForEepromXfer()
{
    auto [value, readCycles] = gba_.ReadMemory(internalSrcAddr_, AccessSize::HALFWORD);
    internalDestAddr_ += static_cast<uint32_t>(AccessSize::HALFWORD);
    internalSrcAddr_ += static_cast<uint32_t>(AccessSize::HALFWORD);
    --internalWordCount_;
    return {value, readCycles};
}
