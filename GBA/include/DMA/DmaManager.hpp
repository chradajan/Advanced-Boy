#pragma once

#include <array>
#include <cstdint>
#include <utility>
#include <DMA/DmaChannel.hpp>
#include <Utilities/MemoryUtilities.hpp>

class GameBoyAdvance;

class DmaManager
{
public:
    /// @brief Register DMA event handling and initialize DMA channels.
    /// @param gba Reference to GBA for accessing memory.
    DmaManager(GameBoyAdvance& gba);

    /// @brief Reset the DMA channels to their power-up states.
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
    void WriteReg(uint32_t addr, uint32_t value, AccessSize alignment);

    /// @brief Check if any of the DMA channels are currently running.
    /// @return True if any channel is active.
    bool DmaActive() const { return dmaActive_; }

    /// @brief Run any DMA channels set to run on VBlank.
    void CheckVBlankChannels() { CheckSpecialTiming(vblank_); }

    /// @brief Run any DMA channels set to run on HBlank.
    void CheckHBlankChannels() { CheckSpecialTiming(hblank_); }

    /// @brief Run any DMA channels set to replenish audio FIFO A.
    void CheckFifoAChannels() { CheckSpecialTiming(fifoA_); }

    /// @brief Run any DMA channels set to replenish audio FIFO B.
    void CheckFifoBChannels() { CheckSpecialTiming(fifoB_); }

private:
    /// @brief Run any DMA channels set to run with special timing.
    /// @param enabledChannels Array of bools indicating which channels should be run based on the current special event.
    void CheckSpecialTiming(std::array<bool, 4>& enabledChannels);

    /// @brief Callback function to resume CPU execution after a DMA transfer completes.
    void EndDma(int) { dmaActive_ = false; }

    /// @brief Schedule when a DMA event should end.
    /// @param cycles Number of cycles until the transfer would finish.
    void HandleDmaEvent(int cycles);

    // Status
    GameBoyAdvance& gba_;
    bool dmaActive_;

    // Channels
    std::array<DmaChannel, 4> dmaChannels_;
    std::array<bool, 4> vblank_;
    std::array<bool, 4> hblank_;
    std::array<bool, 4> fifoA_;
    std::array<bool, 4> fifoB_;
    std::array<bool, 4> videoCapture_;
};
