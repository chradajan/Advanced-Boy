#pragma once

#include <array>
#include <cstdint>
#include <set>

/// @brief 
namespace Graphics
{
constexpr int LCD_WIDTH = 240;
constexpr int LCD_HEIGHT = 160;

enum class PixelSrc : int
{
    OBJ = 0,
    BG0,
    BG1,
    BG2,
    BG3,
    BD
};

struct Pixel
{
    bool operator<(Pixel const& rhs) const;

    uint16_t bgr555_;
    bool transparent_;
    int priority_;
    PixelSrc src_;
};

class FrameBuffer
{
public:
    /// @brief Initialize the frame buffer.
    FrameBuffer();

    /// @brief Access the raw frame buffer data.
    /// @return Raw pointer to frame buffer.
    uint8_t* GetRawFrameBuffer() { return reinterpret_cast<uint8_t*>(frameBuffer_.data()); }

    /// @brief Add a pixel to be considered for drawing to screen.
    /// @param pixel Pixel to potentially draw.
    /// @param dot Index of current scanline to add pixel to.
    void PushPixel(Pixel pixel, int dot);

    /// @brief Iterate through each pixel of current scanline and render the highest priority pixel for each dot.
    /// @param backdrop BGR555 value of backdrop color to use if no pixels were drawn at a location.
    void RenderScanline(uint16_t backdrop);

    /// @brief Reset the frame index to begin drawing at the top of the screen again.
    void ResetFrameIndex() { frameIndex_ = 0; }

private:
    std::array<std::set<Pixel>, LCD_WIDTH> scanline_;
    std::array<uint16_t, LCD_WIDTH * LCD_HEIGHT> frameBuffer_;
    size_t frameIndex_;
};
}
