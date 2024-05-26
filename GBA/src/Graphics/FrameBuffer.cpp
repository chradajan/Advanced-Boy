#include <Graphics/FrameBuffer.hpp>
#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <Graphics/Registers.hpp>

namespace
{
/// @brief Blend the top two layers together.
/// @param eva First target coefficient.
/// @param evb Second target coefficient.
/// @param targetA bgr555 value of first target.
/// @param targetB bgr555 value of second target.
/// @return Blended bgr555 value of the two pixels.
inline uint16_t AlphaBlend(uint16_t eva, uint16_t evb, uint16_t targetA, uint16_t targetB)
{
    // Isolate individual r, g, and b intensities as 1.4 fixed point values
    uint16_t redA = (targetA & 0x001F) << 4;
    uint16_t redB = (targetB & 0x001F) << 4;
    uint16_t greenA = (targetA & 0x03E0) >> 1;
    uint16_t greenB = (targetB & 0x03E0) >> 1;
    uint16_t blueA = (targetA & 0x7C00) >> 6;
    uint16_t blueB = (targetB & 0x7C00) >> 6;

    uint16_t red = ((eva * redA) + (evb * redB)) >> 8;
    uint16_t green = ((eva * greenA) + (evb * greenB)) >> 8;
    uint16_t blue = ((eva * blueA) + (evb * blueB)) >> 8;

    red = std::min(static_cast<uint16_t>(31), red);
    green = std::min(static_cast<uint16_t>(31), green);
    blue = std::min(static_cast<uint16_t>(31), blue);

    return (blue << 10) | (green << 5) | red;
}

/// @brief Increase the brightness of the top layer.
/// @param evy Brightness coefficient.
/// @param target bgr555 value to increase brightness of.
/// @return Increased brightness bgr555 value.
inline uint16_t IncreaseBrightness(uint16_t evy, uint16_t target)
{
    // Isolate individual r, g, and b intensities as 1.4 fixed point values
    uint16_t red = (target & 0x001F) << 4;
    uint16_t green = (target & 0x03E0) >> 1;
    uint16_t blue = (target & 0x7C00) >> 6;

    red = (red + (((0x01F0 - red) * evy) >> 4)) >> 4;
    green = (green + (((0x01F0 - green) * evy) >> 4)) >> 4;
    blue = (blue + (((0x01F0 - blue) * evy) >> 4)) >> 4;

    return (blue << 10) | (green << 5) | red;
}

/// @brief Decrease the brightness of the top layer.
/// @param evy Brightness coefficient.
/// @param target bgr555 value to decrease brightness of.
/// @return Decreased brightness bgr555 value.
inline uint16_t DecreaseBrightness(uint16_t evy, uint16_t target)
{
    // Isolate individual r, g, and b intensities as 1.4 fixed point values
    uint16_t red = (target & 0x001F) << 4;
    uint16_t green = (target & 0x03E0) >> 1;
    uint16_t blue = (target & 0x7C00) >> 6;

    red = (red - ((red * evy) >> 4)) >> 4;
    green = (green - ((green * evy) >> 4)) >> 4;
    blue = (blue - ((blue * evy) >> 4)) >> 4;

    return (blue << 10) | (green << 5) | red;
}
}

namespace Graphics
{
bool Pixel::operator<(Pixel const& rhs) const
{
    if (transparent_ && !rhs.transparent_)
    {
        return false;
    }
    else if (!transparent_ && rhs.transparent_)
    {
        return true;
    }

    if (priority_ == rhs.priority_)
    {
        return src_ < rhs.src_;
    }

    return priority_ < rhs.priority_;
}

FrameBuffer::FrameBuffer()
{
    frameBuffers_[0].fill(0xFFFF);
    frameBuffers_[1].fill(0xFFFF);
    activeFrameBufferIndex_ = 0;
    pixelIndex_ = 0;

    for (auto& pixels : scanline_)
    {
        pixels.reserve(5);
    }
}

uint8_t* FrameBuffer::GetRawFrameBuffer()
{
    return reinterpret_cast<uint8_t*>(frameBuffers_[activeFrameBufferIndex_ ^ 1].data());
}

void FrameBuffer::PushPixel(Pixel pixel, int dot)
{
    scanline_[dot].push_back(pixel);
}

void FrameBuffer::RenderScanline(uint16_t backdropColor, bool forceBlank, BLDCNT const& bldcnt, BLDALPHA const& bldalpha, BLDY const& bldy)
{
    if (forceBlank)
    {
        for (int dot = 0; dot < LCD_WIDTH; ++dot)
        {
            frameBuffers_[activeFrameBufferIndex_].at(pixelIndex_++) = 0x7FFF;
            scanline_[dot].clear();
        }

        return;
    }

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wnarrowing"
    std::array<bool, 6> firstTargetLayer = {bldcnt.flags_.objA, bldcnt.flags_.bg0A, bldcnt.flags_.bg1A, bldcnt.flags_.bg2A, bldcnt.flags_.bg3A, bldcnt.flags_.bdA};
    std::array<bool, 6> secondTargetLayer = {bldcnt.flags_.objB, bldcnt.flags_.bg0B, bldcnt.flags_.bg1B, bldcnt.flags_.bg2B, bldcnt.flags_.bg3B, bldcnt.flags_.bdB};
    #pragma GCC diagnostic pop

    Pixel bdPixel = Pixel(PixelSrc::BD, backdropColor, 4, false);
    Pixel* pixelA = nullptr;
    Pixel* pixelB = nullptr;

    SpecialEffect const bldcntEffect = static_cast<SpecialEffect>(bldcnt.flags_.specialEffect);

    // Coefficients are 1.4 fixed point values
    uint16_t const eva = std::min(bldalpha.flags_.evaCoefficient_, static_cast<uint16_t>(0x10));
    uint16_t const evb = std::min(bldalpha.flags_.evbCoefficient_, static_cast<uint16_t>(0x10));
    uint16_t const evy = std::min(bldy.flags_.evyCoefficient_, static_cast<uint8_t>(0x10));

    for (int dot = 0; dot < LCD_WIDTH; ++dot)
    {
        auto& pixels = scanline_[dot];

        switch (pixels.size())
        {
            case 0:
                pixelA = nullptr;
                pixelB = nullptr;
                break;
            case 1:
                pixelA = &pixels[0];
                pixelB = nullptr;
                break;
            default:
                std::nth_element(pixels.begin(), pixels.begin() + 1, pixels.end());
                pixelA = &pixels[0];
                pixelB = &pixels[1];
                break;
        }

        if (!pixelA || pixelA->transparent_)
        {
            pixelA = &bdPixel;
            pixelB = nullptr;
        }
        else if (pixelB && pixelB->transparent_)
        {
            pixelB = nullptr;
        }

        uint16_t bgr555 = pixelA->bgr555_;
        SpecialEffect actualEffect = bldcntEffect;

        if ((pixelA->semiTransparent_) && pixelB && !pixelB->transparent_)
        {
            actualEffect = SpecialEffect::AlphaBlending;
        }
        else if (!windowScanline_[dot].effectsEnabled_)
        {
            actualEffect = SpecialEffect::None;
        }

        switch (actualEffect)
        {
            case SpecialEffect::None:
                break;
            case SpecialEffect::AlphaBlending:
            {
                if (pixelB && (firstTargetLayer[static_cast<uint8_t>(pixelA->src_)] || pixelA->semiTransparent_) && secondTargetLayer[static_cast<uint8_t>(pixelB->src_)])
                {
                    bgr555 = AlphaBlend(eva, evb, pixelA->bgr555_, pixelB->bgr555_);
                }

                break;
            }
            case SpecialEffect::BrightnessIncrease:
            {
                if (firstTargetLayer[static_cast<uint8_t>(pixelA->src_)])
                {
                    bgr555 = IncreaseBrightness(evy, pixelA->bgr555_);
                }

                break;
            }
            case SpecialEffect::BrightnessDecrease:
            {
                if (firstTargetLayer[static_cast<uint8_t>(pixelA->src_)])
                {
                    bgr555 = DecreaseBrightness(evy, pixelA->bgr555_);
                }

                break;
            }
        }

        frameBuffers_[activeFrameBufferIndex_].at(pixelIndex_++) = bgr555;
        pixels.clear();
    }
}

void FrameBuffer::ResetFrameIndex()
{
    activeFrameBufferIndex_ ^= 1;
    pixelIndex_ = 0;
}

void FrameBuffer::ClearSpritePixels()
{
    for (Pixel& pixel : spriteScanline_)
    {
        pixel.initialized_ = false;
    }
}

void FrameBuffer::PushSpritePixels()
{
    for (size_t dot = 0; dot < LCD_WIDTH; ++dot)
    {
        if (spriteScanline_[dot].initialized_)
        {
            scanline_[dot].push_back(spriteScanline_[dot]);
        }
    }
}
}
