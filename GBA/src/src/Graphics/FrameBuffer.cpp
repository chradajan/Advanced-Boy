#include <Graphics/FrameBuffer.hpp>
#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace Graphics
{
bool Pixel::operator<(Pixel const& rhs) const
{
    if (transparent_ && !rhs.transparent_)
    {
        return true;
    }
    else if (!transparent_ && rhs.transparent_)
    {
        return false;
    }

    if (priority_ == rhs.priority_)
    {
        return src_ > rhs.src_;
    }

    return priority_ > rhs.priority_;
}

FrameBuffer::FrameBuffer()
{
    frameBuffer_.fill(0xFFFF);
    frameIndex_ = 0;

    for (auto& pixels : scanline_)
    {
        pixels.reserve(5);
    }
}

void FrameBuffer::PushPixel(Pixel pixel, int dot)
{
    scanline_[dot].push_back(pixel);
}

void FrameBuffer::RenderScanline(uint16_t backdrop)
{
    for (auto& pixels : scanline_)
    {
        if (pixels.empty())
        {
            frameBuffer_.at(frameIndex_++) = backdrop;
        }
        else
        {
            std::make_heap(pixels.begin(), pixels.end(), std::less<Pixel>());
            Pixel const& pixel = pixels[0];
            uint16_t bgr555 = pixel.transparent_ ? backdrop : pixel.bgr555_;
            frameBuffer_.at(frameIndex_++) = bgr555;
            pixels.clear();
        }
    }
}
}
