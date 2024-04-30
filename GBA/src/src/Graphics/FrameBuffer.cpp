#include <Graphics/FrameBuffer.hpp>
#include <cstdint>
#include <stdexcept>

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
    frameBuffer_.fill(0xFFFF);
    frameIndex_ = 0;
}

void FrameBuffer::PushPixel(Pixel pixel, int dot)
{
    scanline_[dot].insert(pixel);
}

void FrameBuffer::RenderScanline(uint16_t backdrop)
{
    for (auto& pixelSet : scanline_)
    {
        if (pixelSet.empty())
        {
            frameBuffer_.at(frameIndex_++) = backdrop;
        }
        else
        {
            Pixel const& pixel = *pixelSet.begin();
            uint16_t bgr555 = pixel.transparent_ ? backdrop : pixel.bgr555_;
            frameBuffer_.at(frameIndex_++) = bgr555;
            pixelSet.clear();
        }
    }
}
}
