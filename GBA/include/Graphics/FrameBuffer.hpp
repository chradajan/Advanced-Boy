#pragma once

#include <array>
#include <cstdint>
#include <vector>
#include <Graphics/Registers.hpp>

/// @brief 
namespace Graphics
{
constexpr int LCD_WIDTH = 240;
constexpr int LCD_HEIGHT = 160;

enum class PixelSrc : uint8_t
{
    OBJ = 0,
    BG0,
    BG1,
    BG2,
    BG3,
    BD
};

enum class SpecialEffect : uint8_t
{
    None = 0,
    AlphaBlending,
    BrightnessIncrease,
    BrightnessDecrease
};

struct WindowSettings
{
    std::array<bool, 4> bgEnabled_;
    bool objEnabled_;
    bool effectsEnabled_;
};

struct Pixel
{
    /// @brief Default construct a Pixel as uninitialized.
    Pixel() : initialized_(false) {}

    /// @brief Create a Pixel.
    /// @param src Source that generated this pixel.
    /// @param bgr555 Color value of this pixel.
    /// @param priority Priority (0-3) of this pixel.
    /// @param transparent Whether this pixel is transparent.
    /// @param semiTransparent If this is a OBJ pixel, then whether this pixel is part of a semi-transparent sprite.
    Pixel(PixelSrc src, uint16_t bgr555, int priority, bool transparent, bool semiTransparent = false) :
        src_(src),
        bgr555_(bgr555),
        priority_(priority),
        transparent_(transparent),
        semiTransparent_(semiTransparent),
        initialized_(true)
    {
    }

    /// @brief Compare the priority of two pixels. If Pixel A < Pixel B, then A has higher priority and should be drawn over B.
    /// @param rhs Pixel to compare to.
    /// @return True if this pixel has a higher priority than rhs.
    bool operator<(Pixel const& rhs) const;

    PixelSrc src_;
    uint16_t bgr555_;
    int priority_;
    bool transparent_;
    bool semiTransparent_;
    bool initialized_;
};

class FrameBuffer
{
public:
    /// @brief Initialize the frame buffer.
    FrameBuffer();

    /// @brief Access the raw frame buffer data of the last fully drawn frame.
    /// @return Raw pointer to frame buffer.
    uint8_t* GetRawFrameBuffer();

    /// @brief Add a pixel to be considered for drawing to screen.
    /// @param pixel Pixel to potentially draw.
    /// @param dot Index of current scanline to add pixel to.
    void PushPixel(Pixel pixel, int dot);

    /// @brief Iterate through each pixel of current scanline and render the highest priority pixel for each dot.
    /// @param backdropColor BGR555 value of backdrop color to use if no pixels were drawn at a location.
    /// @param windowEnabled Whether to apply window settings when determining special effects.
    /// @param forceBlank Whether to force display a white screen.
    void RenderScanline(uint16_t backdropColor, bool forceBlank, BLDCNT const& bldcnt, BLDALPHA const& bldalpha, BLDY const& bldy);

    /// @brief Reset the frame index to begin drawing at the top of the screen again.
    void ResetFrameIndex();

    /// @brief Uninitialize all pixels in sprite scanline buffer.
    void ClearSpritePixels();

    /// @brief Get a sprite pixel on the current scanline at a particular dot.
    /// @param dot Dot to get sprite pixel at.
    /// @return Reference to pixel at specified dot.
    Pixel& GetSpritePixel(size_t dot) { return spriteScanline_.at(dot); }

    /// @brief Merge the sprite pixels scanline buffer into the main scanline buffer.
    void PushSpritePixels();

    /// @brief Initialize the window settings by setting each pixel to a default setting.
    /// @param outsideSettings Default window settings for each pixel.
    void InitializeWindow(WindowSettings defaultSettings) { windowScanline_.fill(defaultSettings); }

    /// @brief Get the window settings at a dot on the current scanline.
    /// @param dot Index to get window settings at.
    /// @return Window settings at specified dot.
    WindowSettings& GetWindowSettings(size_t dot) { return windowScanline_.at(dot); }

private:
    std::array<std::vector<Pixel>, LCD_WIDTH> scanline_;
    std::array<Pixel, LCD_WIDTH> spriteScanline_;
    std::array<WindowSettings, LCD_WIDTH> windowScanline_;

    std::array<std::array<uint16_t, LCD_WIDTH * LCD_HEIGHT>, 2> frameBuffers_;
    size_t activeFrameBufferIndex_;
    size_t pixelIndex_;
};
}
