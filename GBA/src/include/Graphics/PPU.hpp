#pragma once

#include <Graphics/FrameBuffer.hpp>
#include <Graphics/Registers.hpp>
#include <MemoryMap.hpp>
#include <array>
#include <cstdint>
#include <functional>
#include <utility>

namespace Graphics
{
class PPU
{
public:
    /// @brief Initialize the PPU.
    /// @param paletteRAM Reference to palette RAM.
    /// @param VRAM Reference to VRAM.
    /// @param OAM Reference to OAM.
    PPU(std::array<uint8_t,   1 * KiB> const& paletteRAM,
        std::array<uint8_t,  96 * KiB> const& VRAM,
        std::array<uint8_t,   1 * KiB> const& OAM);

    /// @brief Access the raw frame buffer data.
    /// @return Raw pointer to frame buffer.
    uint8_t* GetRawFrameBuffer() { return frameBuffer_.GetRawFrameBuffer(); }

    /// @brief Read a memory mapped LCD I/O register.
    /// @param addr Address of memory mapped register.
    /// @param accessSize 1 = Byte, 2 = Halfword, 4 = Word.
    /// @return Value of specified register and number of cycles taken to read.
    std::pair<uint32_t, int> ReadLcdReg(uint32_t addr, uint8_t accessSize);

    /// @brief Write a memory mapped LCD I/O register.
    /// @param addr Address of memory mapped register.
    /// @param val Value to write to register.
    /// @param accessSize 1 = Byte, 2 = Halfword, 4 = Word.
    /// @return Number of cycles taken to write.
    int WriteLcdReg(uint32_t addr, uint32_t val, uint8_t accessSize);

private:
    /// @brief Callback function for an enter VDraw event.
    /// @param extraCycles Number of cycles that passed since this event was supposed to execute.
    void VDraw(int extraCycles);

    /// @brief Callback function for an enter HBlank event.
    /// @param extraCycles Number of cycles that passed since this event was supposed to execute.
    void HBlank(int extraCycles);

    /// @brief Callback function for an enter VBlank event.
    /// @param extraCycles Number of cycles that passed since this event was supposed to execute.
    void VBlank(int extraCycles);

    /// @brief Render a scanline in BG Mode 3.
    void RenderMode3Scanline();

    /// @brief Render a scanline in BG Mode 4.
    void RenderMode4Scanline();

    // Frame status
    FrameBuffer frameBuffer_;
    int scanline_;
    int dot_;

    // LCD I/O Registers (0400'0000h - 0400'005Fh)
    std::array<uint8_t, 0x60> lcdRegisters_;
    DISPCNT& lcdControl_;
    DISPSTAT& lcdStatus_;
    VCOUNT& verticalCounter_;

    // Memory
    std::array<uint8_t,   1 * KiB> const& paletteRAM_;  // 0500'0000h - 0500'03FFh    BG/OBJ Palette RAM
    std::array<uint8_t,  96 * KiB> const& VRAM_;        // 0600'0000h - 0601'7FFFh    VRAM - Video RAM
    std::array<uint8_t,   1 * KiB> const& OAM_;         // 0700'0000h - 0700'03FFh    OAM - OBJ Attributes
};
}
