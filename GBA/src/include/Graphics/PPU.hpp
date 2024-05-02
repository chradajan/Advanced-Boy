#pragma once

#include <Graphics/FrameBuffer.hpp>
#include <Graphics/Registers.hpp>
#include <System/MemoryMap.hpp>
#include <array>
#include <cstdint>
#include <functional>
#include <tuple>

namespace Graphics
{
struct OamEntry;

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
    /// @param alignment BYTE, HALFWORD, or WORD.
    /// @return Value of specified register, number of cycles taken to read, and whether this read triggered open bus behavior.
    std::tuple<uint32_t, int, bool> ReadLcdReg(uint32_t addr, AccessSize alignment);

    /// @brief Write a memory mapped LCD I/O register.
    /// @param addr Address of memory mapped register.
    /// @param val Value to write to register.
    /// @param alignment BYTE, HALFWORD, or WORD.
    /// @return Number of cycles taken to write.
    int WriteLcdReg(uint32_t addr, uint32_t value, AccessSize alignment);

    /// @brief Return the current background mode.
    /// @return Current background mode.
    uint8_t BgMode() const { return lcdControl_.flags_.bgMode; }

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

    /// @brief Render a scanline in BG Mode 0.
    void RenderMode0Scanline();

    /// @brief Render a scanline in BG Mode 3.
    void RenderMode3Scanline();

    /// @brief Render a scanline in BG Mode 4.
    void RenderMode4Scanline();

    /// @brief Render sprites and mix with background.
    void RenderSprites();

    /// @brief Render a tiled text background scanline.
    /// @param bgIndex Which background to render.
    /// @param control Control register of specified background.
    /// @param xOffset X offset register value of specified background.
    /// @param yOffset Y offset register value of specified background.
    void RenderRegularTiledBackgroundScanline(int bgIndex, BGCNT const& control, int xOffset, int yOffset);

    /// @brief Render a one dimensional 4bpp sprite into an array of pixels.
    /// @param x X-coordinate of top left corner of sprite.
    /// @param y Y-coordinate of top left corner of sprite.
    /// @param width Width of sprite in pixels.
    /// @param height Height of sprite in pixels.
    /// @param oamEntry Reference to OAM entry for sprite.
    /// @param pixels Array of pixels to draw to. Any pixels already in array are from a sprite with a lower OAM index.
    void Render1d4bppSprite(int x, int y, int width, int height, OamEntry const& oamEntry, std::array<Pixel, 240>& pixels);

    /// @brief Render a one dimensional 8bpp sprite into an array of pixels.
    /// @param x X-coordinate of top left corner of sprite.
    /// @param y Y-coordinate of top left corner of sprite.
    /// @param width Width of sprite in pixels.
    /// @param height Height of sprite in pixels.
    /// @param oamEntry Reference to OAM entry for sprite.
    /// @param pixels Array of pixels to draw to. Any pixels already in array are from a sprite with a lower OAM index.
    void Render1d8bppSprite(int x, int y, int width, int height, OamEntry const& oamEntry, std::array<Pixel, 240>& pixels);

    /// @brief Render a two dimensional 4bpp sprite into an array of pixels.
    /// @param x X-coordinate of top left corner of sprite.
    /// @param y Y-coordinate of top left corner of sprite.
    /// @param width Width of sprite in pixels.
    /// @param height Height of sprite in pixels.
    /// @param oamEntry Reference to OAM entry for sprite.
    /// @param pixels Array of pixels to draw to. Any pixels already in array are from a sprite with a lower OAM index.
    void Render2d4bppSprite(int x, int y, int width, int height, OamEntry const& oamEntry, std::array<Pixel, 240>& pixels);

    /// @brief Render a two dimensional 8bpp sprite into an array of pixels.
    /// @param x X-coordinate of top left corner of sprite.
    /// @param y Y-coordinate of top left corner of sprite.
    /// @param width Width of sprite in pixels.
    /// @param height Height of sprite in pixels.
    /// @param oamEntry Reference to OAM entry for sprite.
    /// @param pixels Array of pixels to draw to. Any pixels already in array are from a sprite with a lower OAM index.
    void Render2d8bppSprite(int x, int y, int width, int height, OamEntry const& oamEntry, std::array<Pixel, 240>& pixels);

    // Frame status
    FrameBuffer frameBuffer_;
    uint8_t scanline_;

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
