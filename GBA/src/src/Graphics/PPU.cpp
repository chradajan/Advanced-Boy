#include <Graphics/PPU.hpp>
#include <Graphics/VramTypes.hpp>
#include <System/InterruptManager.hpp>
#include <System/MemoryMap.hpp>
#include <System/Scheduler.hpp>
#include <System/Utilities.hpp>
#include <algorithm>
#include <cstdint>
#include <format>
#include <functional>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace Graphics
{
static_assert(sizeof(TileMap) == TEXT_TILE_MAP_SIZE);
static_assert(sizeof(TileMapEntry) == 2);
static_assert(sizeof(TileData8bpp) == 64);
static_assert(sizeof(TileData4bpp) == 32);

PPU::PPU(std::array<uint8_t,   1 * KiB> const& paletteRAM,
         std::array<uint8_t,  96 * KiB> const& VRAM,
         std::array<uint8_t,   1 * KiB> const& OAM) :
    frameBuffer_(),
    lcdRegisters_(),
    lcdControl_(*reinterpret_cast<DISPCNT*>(&lcdRegisters_.at(0))),
    lcdStatus_(*reinterpret_cast<DISPSTAT*>(&lcdRegisters_.at(4))),
    verticalCounter_(*reinterpret_cast<VCOUNT*>(&lcdRegisters_.at(6))),
    paletteRAM_(paletteRAM),
    VRAM_(VRAM),
    OAM_(OAM)
{
    scanline_ = 0;
    dot_ = 0;
    lcdRegisters_.fill(0);

    Scheduler.RegisterEvent(EventType::HBlank, std::bind(&HBlank, this, std::placeholders::_1));
    Scheduler.RegisterEvent(EventType::VBlank, std::bind(&VBlank, this, std::placeholders::_1));
    Scheduler.RegisterEvent(EventType::VDraw, std::bind(&VDraw, this, std::placeholders::_1));

    Scheduler.ScheduleEvent(EventType::HBlank, 960);
}

std::tuple<uint32_t, int, bool> PPU::ReadLcdReg(uint32_t addr, AccessSize alignment)
{
    addr = AlignAddress(addr, alignment);

    if (((addr >= BG0HOFS_ADDR) && (addr < WININ_ADDR)) ||
        ((addr >= MOSAIC_ADDR) && (addr < BLDCNT_ADDR)) ||
        (addr >= BLDY_ADDR))
    {
        // Write-only or unused regions
        return {0, 1, true};
    }

    size_t index = addr - LCD_IO_ADDR_MIN;
    uint8_t* bytePtr = &(lcdRegisters_.at(index));
    uint32_t value = ReadPointer(bytePtr, alignment);
    return {value, 1, false};
}

int PPU::WriteLcdReg(uint32_t addr, uint32_t value, AccessSize alignment)
{
    addr = AlignAddress(addr, alignment);

    if ((addr >= DISPSTAT_ADDR) && (addr < VCOUNT_ADDR))
    {
        // Write to DISPSTAT; not all bits are writable. If writing a word, the next register is VCOUNT so don't write that either.
        static constexpr uint16_t DISPSTAT_WRITABLE_MASK = 0b1111'1111'1011'1000;
        value &= DISPSTAT_WRITABLE_MASK;

        if (alignment == AccessSize::BYTE)
        {
            value &= MAX_U8;
            lcdStatus_ = (lcdStatus_.halfword_ & ~(DISPSTAT_WRITABLE_MASK & MAX_U8)) | value;
        }
        else
        {
            lcdStatus_.halfword_ = (lcdStatus_.halfword_ & ~DISPSTAT_WRITABLE_MASK) | value;
        }

        return 1;
    }
    else if ((addr >= VCOUNT_ADDR) && (addr < BG0CNT_ADDR))
    {
        // Write to VCOUNT which is read-only. VCOUNT is halfword aligned, so alignment must be byte or halfword.
        return 1;
    }

    size_t index = addr - LCD_IO_ADDR_MIN;
    uint8_t* bytePtr = &(lcdRegisters_.at(index));
    WritePointer(bytePtr, value, alignment);
    return 1;
}

void PPU::VDraw(int extraCycles)
{
    // VDraw register updates
    ++scanline_;

    if (scanline_ == 228)
    {
        scanline_ = 0;
        lcdStatus_.flags_.vBlank = 0;
    }

    verticalCounter_.flags_.currentScanline = scanline_;
    lcdStatus_.flags_.hBlank = 0;

    if (scanline_ == lcdStatus_.flags_.vCountSetting)
    {
        lcdStatus_.flags_.vCounter = 1;

        if (lcdStatus_.flags_.vCounterIrqEnable)
        {
            InterruptMgr.RequestInterrupt(InterruptType::LCD_VCOUNTER_MATCH);
        }
    }
    else
    {
        lcdStatus_.flags_.vCounter = 0;
    }

    // Event Scheduling
    int cyclesUntilHBlank = (960 - extraCycles) + 46;
    Scheduler.ScheduleEvent(EventType::HBlank, cyclesUntilHBlank);
}

void PPU::HBlank(int extraCycles)
{
    // HBlank register updates
    lcdStatus_.flags_.hBlank = 1;

    if (lcdStatus_.flags_.hBlankIrqEnable)
    {
        InterruptMgr.RequestInterrupt(InterruptType::LCD_HBLANK);
    }

    // Event scheduling
    int cyclesUntilNextEvent = 226 - extraCycles;
    EventType nextEvent = (scanline_ == 159) ? EventType::VBlank : EventType::VDraw;
    Scheduler.ScheduleEvent(nextEvent, cyclesUntilNextEvent);
    uint16_t backdrop = *reinterpret_cast<uint16_t const*>(paletteRAM_.data());

    // Draw scanline
    if (lcdControl_.flags_.forceBlank)
    {
        backdrop = 0xFFFF;
    }
    else
    {
        switch (lcdControl_.flags_.bgMode)
        {
            case 0:
                RenderMode0Scanline();
                break;
            case 3:
                RenderMode3Scanline();
                break;
            case 4:
                RenderMode4Scanline();
                break;
            default:
                backdrop = 0xFFFF;
                break;
        }
    }

    frameBuffer_.RenderScanline(backdrop);
}

void PPU::VBlank(int extraCycles)
{
    // VBlank register updates
    ++scanline_;
    verticalCounter_.flags_.currentScanline = scanline_;

    if (scanline_ == 160)
    {
        // First time entering VBlank
        lcdStatus_.flags_.vBlank = 1;
        lcdStatus_.flags_.hBlank = 0;
        Scheduler.ScheduleEvent(EventType::REFRESH_SCREEN, SCHEDULE_NOW);
        frameBuffer_.ResetFrameIndex();

        if (lcdStatus_.flags_.vBlankIrqEnable)
        {
            InterruptMgr.RequestInterrupt(InterruptType::LCD_VBLANK);
        }
    }
    else if (scanline_ == 227)
    {
        lcdStatus_.flags_.vBlank = 0;
    }

    if (scanline_ == lcdStatus_.flags_.vCountSetting)
    {
        lcdStatus_.flags_.vCounter = 1;

        if (lcdStatus_.flags_.vCounterIrqEnable)
        {
            InterruptMgr.RequestInterrupt(InterruptType::LCD_VCOUNTER_MATCH);
        }
    }
    else
    {
        lcdStatus_.flags_.vCounter = 0;
    }

    // Event Scheduling
    int cyclesUntilNextEvent = 1232 - extraCycles;
    EventType nextEvent = (scanline_ = 227) ? EventType::VDraw : EventType::VBlank;
    Scheduler.ScheduleEvent(nextEvent, cyclesUntilNextEvent);
}

void PPU::RenderMode0Scanline()
{
    for (uint16_t i = 0; i < 4; ++i)
    {
        if (lcdControl_.halfword_ & (0x100 << i))
        {
            BGCNT const& bgControl = *reinterpret_cast<BGCNT*>(&lcdRegisters_[0x08 + (2 * i)]);
            uint16_t xOffset = *reinterpret_cast<uint16_t*>(&lcdRegisters_[0x10 + (4 * i)]) & 0x01FF;
            uint16_t yOffset = *reinterpret_cast<uint16_t*>(&lcdRegisters_[0x12 + (4 * i)]) & 0x01FF;
            RenderRegularTiledBackgroundScanline(i, bgControl, xOffset, yOffset);
        }
    }
}

void PPU::RenderMode3Scanline()
{
    size_t vramIndex = scanline_ * 480;
    uint16_t const* vramPtr = reinterpret_cast<uint16_t const*>(&VRAM_.at(vramIndex));

    if (lcdControl_.flags_.screenDisplayBg2)
    {
        for (int i = 0; i < 240; ++i)
        {
            frameBuffer_.PushPixel({*vramPtr, false, 0, PixelSrc::BG2}, i);
            ++vramPtr;
        }
    }
}

void PPU::RenderMode4Scanline()
{
    size_t vramIndex = scanline_ * 240;
    uint16_t const* palettePtr = reinterpret_cast<uint16_t const*>(paletteRAM_.data());

    if (lcdControl_.flags_.displayFrameSelect)
    {
        vramIndex += 0xA000;
    }

    if (lcdControl_.flags_.screenDisplayBg2)
    {
        for (int i = 0; i < 240; ++i)
        {
            uint8_t paletteIndex = VRAM_.at(vramIndex++);
            uint16_t bgr555 = palettePtr[paletteIndex];
            frameBuffer_.PushPixel({bgr555, !(paletteIndex & 0x0F), 0, PixelSrc::BG2}, i);
        }
    }
}

void PPU::RenderRegularTiledBackgroundScanline(int bgIndex, BGCNT const& control, int xOffset, int yOffset)
{
    // Map size in pixels
    int const mapPixelWidth = (control.screenSize_ & 0x01) ? 512 : 256;
    int const mapPixelHeight = (control.screenSize_ & 0x02) ? 512 : 256;

    // Pixel coordinates
    int dotX = xOffset % mapPixelWidth;
    int const dotY = (scanline_ + yOffset) % mapPixelHeight;

    // Screen entry coordinates
    int mapX = dotX / 8;
    int mapY = dotY / 8;

    // Screenblock/Charblock addresses
    size_t tileMapAddr = control.screenBaseBlock_ * TEXT_TILE_MAP_SIZE;
    size_t baseCharblockAddr = control.charBaseBlock_ * CHARBLOCK_SIZE;

    if (mapY > 31)
    {
        tileMapAddr += (TEXT_TILE_MAP_SIZE * ((mapPixelWidth == 256) ? 1 : 2));
        mapY %= 32;
    }

    // Memory pointers
    TileMapEntry const* tileMapEntryPtr_ = nullptr;
    uint16_t const* const palettePtr = reinterpret_cast<uint16_t const*>(paletteRAM_.data());

    // Status
    int totalPixelsDrawn = 0;
    int dot = 0;
    int priority = control.bgPriority_;
    PixelSrc src = static_cast<PixelSrc>(bgIndex + 1);

    while (totalPixelsDrawn < 240)
    {
        if (tileMapEntryPtr_ == nullptr)
        {
            mapX = dotX / 8;
            TileMap const* tileMapPtr_ = reinterpret_cast<TileMap const*>(&VRAM_.at(tileMapAddr));

            if (mapX > 31)
            {
                ++tileMapPtr_;
                mapX %= 32;
            }

            tileMapEntryPtr_ = &(tileMapPtr_->tileData_[mapY][mapX]);
        }

        int tileX = dotX % 8;
        int tileY = dotY % 8;
        int pixelsToDraw = std::min(8 - tileX, 240 - totalPixelsDrawn);
        int pixelsDrawn = 0;

        if (tileMapEntryPtr_->verticalFlip_)
        {
            tileY ^= 7;
        }

        if (control.colorMode_)
        {
            // 8bpp
            TileData8bpp const* tilePtr = reinterpret_cast<TileData8bpp const*>(&VRAM_.at(baseCharblockAddr));

            if (tileMapEntryPtr_->horizontalFlip_)
            {
                tileX ^= 7;
            }

            while (pixelsDrawn < pixelsToDraw)
            {
                size_t paletteIndex = tilePtr[tileMapEntryPtr_->tile_].paletteIndex_[tileY][tileX];
                uint16_t bgr555 = palettePtr[paletteIndex];
                bool transparent = !(paletteIndex & 0x0F);
                frameBuffer_.PushPixel({bgr555, transparent, priority, src}, dot);
                ++tileX;
                ++pixelsDrawn;
                ++dot;
            }
        }
        else
        {
            // 4bpp
            TileData4bpp const* tilePtr = reinterpret_cast<TileData4bpp const*>(&VRAM_.at(baseCharblockAddr));

            if (tileMapEntryPtr_->horizontalFlip_)
            {
                tileX ^= 7;
            }

            bool leftHalf = (tileX % 2) == 0;

            while (pixelsDrawn < pixelsToDraw)
            {
                size_t paletteIndex = tileMapEntryPtr_->palette_ << 4;

                if (leftHalf)
                {
                    paletteIndex |= tilePtr[tileMapEntryPtr_->tile_].paletteIndex_[tileY][tileX / 2].leftNibble_;
                }
                else
                {
                    paletteIndex |= tilePtr[tileMapEntryPtr_->tile_].paletteIndex_[tileY][tileX / 2].rightNibble_;
                }

                uint16_t bgr555 = palettePtr[paletteIndex];
                bool transparent = !(paletteIndex & 0x0F);
                frameBuffer_.PushPixel({bgr555, transparent, priority, src}, dot);
                ++tileX;
                ++pixelsDrawn;
                ++dot;
                leftHalf = !leftHalf;
            }
        }

        int nextDotX = (dotX + pixelsDrawn) % mapPixelWidth;

        if ((nextDotX / 256) != (dotX / 256))
        {
            tileMapEntryPtr_ = nullptr;
        }
        else
        {
            ++tileMapEntryPtr_;
        }

        dotX = nextDotX;
        totalPixelsDrawn += pixelsDrawn;
    }
}
}
