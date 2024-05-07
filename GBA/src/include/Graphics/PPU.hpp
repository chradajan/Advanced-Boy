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

    /// @brief Check the current scanline being processed.
    /// @return Current scanline [0, 227].
    int CurrentScanline() const { return scanline_; }

    /// @brief Return the current background mode.
    /// @return Current background mode.
    uint8_t BgMode() const { return lcdControl_.flags_.bgMode; }

    /// @brief Function to call on HBlank events.
    /// @param extraCycles Number of cycles that passed since this event was supposed to execute.
    void HBlank(int extraCycles);

    /// @brief Function to call on VBlank events.
    /// @param extraCycles Number of cycles that passed since this event was supposed to execute.
    void VBlank(int extraCycles);

private:
    /// @brief Callback function for an enter VDraw event.
    /// @param extraCycles Number of cycles that passed since this event was supposed to execute.
    void VDraw(int extraCycles);

    /// @brief Determine whether window 0 and window 1 are active on the current scanline.
    void SetNonObjWindowEnabled();

    /// @brief Apply window settings to pixels within a window on the current scanline.
    /// @param leftEdge X1 - Left edge of window (inclusive).
    /// @param rightEdge X2 - Right edge of window (exclusive).
    /// @param settings Settings for inside this window region.
    void ConfigureNonObjWindow(uint8_t leftEdge, uint8_t rightEdge, WindowSettings settings);

    /// @brief Render BG pixels in mode 0.
    void RenderMode0Scanline();

    /// @brief Render BG pixels in mode 1.
    void RenderMode1Scanline();

    /// @brief Render BG pixels in mode 2.
    void RenderMode2Scanline();

    /// @brief Render BG pixels in mode 3.
    void RenderMode3Scanline();

    /// @brief Render BG pixels in mode 4.
    void RenderMode4Scanline();

    /// @brief Render a regular tiled text background scanline.
    /// @param bgIndex Which background to render.
    /// @param control Control register of specified background.
    /// @param xOffset X offset register value of specified background.
    /// @param yOffset Y offset register value of specified background.
    void RenderRegularTiledBackgroundScanline(int bgIndex, BGCNT const& control, int xOffset, int yOffset);

    /// @brief Render a regular tiled text background scanline that uses 4bpp colors.
    /// @param bgIndex Which background to render.
    /// @param control Control register of specified background.
    /// @param x X-coordinate within background map.
    /// @param y Y-Coordinate within background map.
    /// @param width Width of map.
    void RenderRegular4bppBackground(int bgIndex, BGCNT const& control, int x, int y, int width);

    /// @brief Render a regular tiled text background scanline that uses 8bpp colors.
    /// @param bgIndex Which background to render.
    /// @param control Control register of specified background.
    /// @param x X-coordinate within background map.
    /// @param y Y-Coordinate within background map.
    /// @param width Width of map.
    void RenderRegular8bppBackground(int bgIndex, BGCNT const& control, int x, int y, int width);

    /// @brief Render a tiled affined background scanline.
    /// @param bgIndex Which background to render.
    /// @param control Control register of specified background.
    /// @param dx Reference point x-coordinate.
    /// @param dy Reference point y-coordinate.
    /// @param pa Amount to increment x-coordinate by after each pixel.
    /// @param pc Amount to increment y-coordinate by after each pixel.
    void RenderAffineTiledBackgroundScanline(int bgIndex, BGCNT const& control, int32_t dx, int32_t dy, int16_t pa, int16_t pc);

    /// @brief Render sprites and mix with background.
    /// @param windowSettingsPtr Pointer to OBJ window settings, or nullptr if rendering a visible sprites.
    void EvaluateOAM(WindowSettings* windowSettingsPtr = nullptr);

    /// @brief Render a one dimensional 4bpp sprite into an array of pixels.
    /// @param x X-coordinate of top left corner of sprite.
    /// @param y Y-coordinate of top left corner of sprite.
    /// @param width Width of sprite in pixels.
    /// @param height Height of sprite in pixels.
    /// @param oamEntry Reference to OAM entry for sprite.
    /// @param windowSettingsPtr Pointer to OBJ window settings, or nullptr if rendering a visible sprite.
    void Render1d4bppSprite(int x, int y, int width, int height, OamEntry const& oamEntry, WindowSettings* windowSettingsPtr);

    /// @brief Render a one dimensional 8bpp sprite into an array of pixels.
    /// @param x X-coordinate of top left corner of sprite.
    /// @param y Y-coordinate of top left corner of sprite.
    /// @param width Width of sprite in pixels.
    /// @param height Height of sprite in pixels.
    /// @param oamEntry Reference to OAM entry for sprite.
    /// @param windowSettingsPtr Pointer to OBJ window settings, or nullptr if rendering a visible sprite.
    void Render1d8bppSprite(int x, int y, int width, int height, OamEntry const& oamEntry, WindowSettings* windowSettingsPtr);

    /// @brief Render a two dimensional 4bpp sprite into an array of pixels.
    /// @param x X-coordinate of top left corner of sprite.
    /// @param y Y-coordinate of top left corner of sprite.
    /// @param width Width of sprite in pixels.
    /// @param height Height of sprite in pixels.
    /// @param oamEntry Reference to OAM entry for sprite.
    /// @param windowSettingsPtr Pointer to OBJ window settings, or nullptr if rendering a visible sprite.
    void Render2d4bppSprite(int x, int y, int width, int height, OamEntry const& oamEntry, WindowSettings* windowSettingsPtr);

    /// @brief Render a two dimensional 8bpp sprite into an array of pixels.
    /// @param x X-coordinate of top left corner of sprite.
    /// @param y Y-coordinate of top left corner of sprite.
    /// @param width Width of sprite in pixels.
    /// @param height Height of sprite in pixels.
    /// @param oamEntry Reference to OAM entry for sprite.
    /// @param windowSettingsPtr Pointer to OBJ window settings, or nullptr if rendering a visible sprite.
    void Render2d8bppSprite(int x, int y, int width, int height, OamEntry const& oamEntry, WindowSettings* windowSettingsPtr);

    /// @brief Increment BG2 and BG3 reference points after a scanline is rendered.
    void IncrementAffineBackgroundReferencePoints();

    // Frame status
    FrameBuffer frameBuffer_;
    uint8_t scanline_;
    bool window0EnabledOnScanline_;
    bool window1EnabledOnScanline_;

    // Affine background reference points
    int32_t bg2RefX_;
    int32_t bg2RefY_;
    int32_t bg3RefX_;
    int32_t bg3RefY_;

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
