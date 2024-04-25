#include <Graphics/FrameBuffer.hpp>
#include <cstdint>

namespace Graphics
{
bool Pixel::operator<(Pixel const& rhs) const
{
    if (transparent_ && !rhs.transparent_)
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
            frameBuffer_.at(frameIndex_++) = pixelSet.begin()->bgr555_;
            pixelSet.clear();
        }
    }
}
}
