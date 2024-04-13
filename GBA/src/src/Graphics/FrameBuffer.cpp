#include <Graphics/FrameBuffer.hpp>
#include <cstdint>

namespace Graphics
{
FrameBuffer::FrameBuffer()
{
    frameBuffer_.fill(0);
    frameIndex_ = 0;
}

void FrameBuffer::WritePixel(uint16_t rgb555)
{
    uint8_t r = rgb555 & 0x001F;
    r = (r << 3) | (r >> 2);

    uint8_t g = (rgb555 & 0x03E0) >> 5;
    g = (g << 3) | (g >> 2);

    uint8_t b = (rgb555 & 0x7C00) >> 10;
    b = (b << 3) | (b >> 2);

    frameBuffer_.at(frameIndex_++) = r;
    frameBuffer_.at(frameIndex_++) = g;
    frameBuffer_.at(frameIndex_++) = b;
}
}
