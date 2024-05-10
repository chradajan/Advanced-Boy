#pragma once

#include <array>
#include <cstdint>
#include <tuple>
#include <System/InterruptManager.hpp>
#include <System/MemoryMap.hpp>
#include <System/Scheduler.hpp>

class GameBoyAdvance;

class DmaChannel
{
public:
    /// @brief Initialize a DMA channel's registers and assign it an event and interrupt based on its index.
    /// @param index Which DMA channel this is.
    DmaChannel(int index);

    /// @brief Read a register associated with this DMA channel.
    /// @param addr Address of memory mapped register.
    /// @param alignment BYTE, HALFWORD, or WORD.
    /// @return Value of specified register, number of cycles taken to read, and whether this read triggered open bus behavior.
    std::tuple<uint32_t, int, bool> ReadRegister(uint32_t addr, AccessSize alignment);

    /// @brief Write a register associated with this DMA channel.
    /// @param addr Address of memory mapped register.
    /// @param value Value to write to register.
    /// @param alignment BYTE, HALFWORD, or WORD.
    /// @param gbaPtr Pointer to GBA needed to schedule DMA events.
    /// @return Number of cycles taken to write.
    int WriteRegister(uint32_t addr, uint32_t value, AccessSize alignment, GameBoyAdvance* gbaPtr);

    /// @brief Run DMA transfer until internalWordCount_ hits 0. Then disable this channel or set it up to repeat.
    /// @param gbaPtr Pointer to GBA needed to read/write memory during transfer.
    void Execute(GameBoyAdvance* gbaPtr);

    /// @brief Partially run a DMA transfer in case this channel was interrupted by a higher priority channel.
    /// @param gbaPtr Pointer to GBA needed to read/write memory during transfer.
    /// @param cycles How many cycles this transfer was executing for before being interrupted.
    void PartiallyExecute(GameBoyAdvance* gbaPtr, int cycles);

    /// @brief Determine how many cycles this channel will take to complete a transfer.
    /// @param gbaPtr Pointer to GBA needed to determine read timing in ROM.
    /// @return How many cycles this channel would take to fully execute.
    int TransferCycleCount(GameBoyAdvance* gbaPtr);

    /// @brief Get the DMA event associated with this channel.
    /// @return DMA event.
    EventType GetDmaEvent() const { return dmaEvent_; }

private:
    /// @brief Either start this channel immediately or set it up to execute on its start condition.
    /// @param gbaPtr Pointer to GBA needed to schedule a transfer.
    void ScheduleTransfer(GameBoyAdvance* gbaPtr);

    /// @brief Set how many words should be transferred.
    void SetInternalWordCount();

    /// @brief Update the internal src and dest addresses after each word transfer.
    void UpdateInternalAddresses();

    union DMAXCNT
    {
        struct
        {
            uint32_t wordCount_ : 16;
            uint32_t : 5;
            uint32_t destAddrControl_ : 2;
            uint32_t srcAddrControl_ : 2;
            uint32_t repeat_ : 1;
            uint32_t xferType_ : 1;
            uint32_t gamePakDRQ_ : 1;
            uint32_t startTiming_ : 2;
            uint32_t irqEnable_ : 1;
            uint32_t dmaEnable_ : 1;
        } flags_;

        uint32_t word_;
    };

    // Memory mapped registers
    std::array<uint8_t, 12> dmaChannelRegisters_;
    uint32_t const& SAD_;
    uint32_t const& DAD_;
    DMAXCNT& dmaControl_;

    // Internal registers
    uint32_t internalSrcAddr_;
    uint32_t internalDestAddr_;
    uint32_t internalWordCount_;
    AccessSize xferAlignment_;

    // Status
    int dmaChannelIndex_;
    int totalTransferCycles_;
    int cyclesPerTransfer_;
    bool eepromRead_;
    bool eepromWrite_;

    // Scheduling
    EventType dmaEvent_;
    InterruptType dmaInterrupt_;
};
