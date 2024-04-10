#pragma once

#include <array>
#include <cstdint>

namespace Graphics
{
constexpr int LCD_WIDTH = 240;
constexpr int LCD_HEIGHT = 160;
constexpr int LCD_CHANNELS = 3;

class FrameBuffer
{
public:
    /// @brief Initialize the frame buffer.
    FrameBuffer();

    /// @brief Access the raw frame buffer data.
    /// @return Raw pointer to frame buffer.
    uint8_t* GetRawFrameBuffer() { return frameBuffer_.data(); }

    /// @brief Write a pixel into the frame buffer.
    /// @param rgb555 RGB555 pixel data.
    void WritePixel(uint16_t rgb555);

    /// @brief Reset the frame index to begin drawing at the top of the screen again.
    void ResetFrameIndex() { frameIndex_ = 0; }

private:
    std::array<uint8_t, LCD_WIDTH * LCD_HEIGHT * LCD_CHANNELS> frameBuffer_;
    size_t frameIndex_;
};
}
