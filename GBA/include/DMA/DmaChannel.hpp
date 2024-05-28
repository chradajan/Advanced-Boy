#pragma once

#include <array>
#include <cstdint>
#include <utility>
#include <System/SystemControl.hpp>
#include <Utilities/MemoryUtilities.hpp>

class GameBoyAdvance;

enum class DmaXfer
{
    NO_CHANGE,
    DISABLE,
    IMMEDIATE,
    VBLANK,
    HBLANK,
    FIFO_A,
    FIFO_B,
    VIDEO_CAPTURE
};

class DmaChannel
{
public:
    /// @brief Initialize a DMA channel
    /// @param index Which channel index this is.
    /// @param interrupt What interrupt type should be requested when this channel executes, if IRQs are enabled.
    /// @param gba Reference to GBA for accessing memory.
    DmaChannel(int index, InterruptType interrupt, GameBoyAdvance& gba);

    /// @brief Reset to power-up state.
    void Reset();

    /// @brief Read a DMA register.
    /// @param addr Address of register to read.
    /// @param alignment Number of bytes to read.
    /// @return Value of specified register and whether this read triggered open bus behavior.
    std::pair<uint32_t, bool> ReadReg(uint32_t addr, AccessSize alignment);

    /// @brief Write a DMA register.
    /// @param addr Address of register to write.
    /// @param value Value to write to register.
    /// @param alignment Number of bytes to write.
    DmaXfer WriteReg(uint32_t addr, uint32_t value, AccessSize alignment);

    /// @brief Check if this channel is currently enabled.
    /// @return True if this channel is running or scheduled to run with special timing.
    bool Enabled() const { return dmacnt_.enable; }

    /// @brief Execute a DMA transfer based on this channel's current configuration.
    /// @return Number of cycles taken to complete the transfer.
    int Execute();

private:
    /// @brief Execute a regular DMA transfer.
    /// @return Number of cycles taken to complete the transfer.
    int ExecuteNormalXfer();

    /// @brief Execute a DMA transfer to/from EEPROM.
    /// @param read True if this transfer is set to read from EEPROM and write to system memory.
    /// @param write True if this transfer is set to read from system memory and write to EEPROM.
    /// @return Number of cycles taken to complete the transfer.
    int ExecuteEepromXfer(bool read, bool write);

    /// @brief Execute a DMA transfer to replenish an audio FIFO.
    /// @return Number of cycles taken to complete the transfer.
    int ExecuteFifoXfer();

    /// @brief Determine if this channel is configured to transfer data to an audio FIFO.
    /// @return True if this is an audio FIFO channel.
    bool IsFifoXfer() const;

    /// @brief Determine when this channel should execute.
    /// @return Transfer type for this channel.
    DmaXfer DetermineStartTiming() const;

    /// @brief Set the internal address and word count registers.
    void SetInternalRegisters();

    /// @brief Read a bit from the bitstream in system memory that's being written to EEPROM. Updates internal registers.
    /// @return Current value at internal source address and number of cycles taken to read.
    std::pair<uint32_t, int> ReadForEepromXfer();

    union DMACNT
    {
        struct
        {
            uint16_t : 5;
            uint16_t destAddrCnt : 2;
            uint16_t srcAddrCnt : 2;
            uint16_t repeat : 1;
            uint16_t xferType : 1;
            uint16_t gamePakDrq : 1;
            uint16_t startTiming : 2;
            uint16_t irq : 1;
            uint16_t enable : 1;
        };

        uint16_t value;
    };


    // DMA registers
    std::array<uint8_t, 12> dmaRegisters_;
    uint32_t& sad_;
    uint32_t& dad_;
    uint16_t& wordCount_;
    DMACNT& dmacnt_;

    // Internal registers
    uint32_t internalSrcAddr_;
    uint32_t internalDestAddr_;
    uint32_t internalWordCount_;

    // DMA Channel info
    int const channelIndex_;
    InterruptType const interruptType_;
    GameBoyAdvance& gba_;
};
