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
static_assert(sizeof(ScreenBlock) == SCREENBLOCK_SIZE);
static_assert(sizeof(ScreenBlockEntry) == 2);
static_assert(sizeof(TileData8bpp) == 64);
static_assert(sizeof(TileData4bpp) == 32);
static_assert(sizeof(OamEntry) == 8);
static_assert(sizeof(TwoDim4bppMap) == 2 * CHARBLOCK_SIZE);
static_assert(sizeof(TwoDim8bppMap) == 2 * CHARBLOCK_SIZE);

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
    EventType nextEvent = (scanline_ < 159) ? EventType::VDraw : EventType::VBlank;
    Scheduler.ScheduleEvent(nextEvent, cyclesUntilNextEvent);

    // Draw scanline if not in VBlank
    if (scanline_ < 160)
    {
        uint16_t backdrop = *reinterpret_cast<uint16_t const*>(paletteRAM_.data());

        if (lcdControl_.flags_.forceBlank)
        {
            backdrop = 0xFFFF;
        }
        else
        {
            if (lcdControl_.flags_.screenDisplayObj)
            {
                RenderSprites();
            }

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
}

void PPU::VBlank(int extraCycles)
{
    // VBlank register updates
    ++scanline_;
    verticalCounter_.flags_.currentScanline = scanline_;
    lcdStatus_.flags_.hBlank = 0;

    if (scanline_ == 160)
    {
        // First time entering VBlank
        lcdStatus_.flags_.vBlank = 1;
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
    Scheduler.ScheduleEvent(EventType::HBlank, (960 - extraCycles) + 46);
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
    BGCNT const& bgControl = *reinterpret_cast<BGCNT*>(&lcdRegisters_[0x0C]);
    size_t vramIndex = scanline_ * 480;
    uint16_t const* vramPtr = reinterpret_cast<uint16_t const*>(&VRAM_.at(vramIndex));

    if (lcdControl_.flags_.screenDisplayBg2)
    {
        for (int i = 0; i < 240; ++i)
        {
            frameBuffer_.PushPixel({PixelSrc::BG2, *vramPtr, bgControl.flags_.bgPriority_, false}, i);
            ++vramPtr;
        }
    }
}

void PPU::RenderMode4Scanline()
{
    BGCNT const& bgControl = *reinterpret_cast<BGCNT*>(&lcdRegisters_[0x0C]);
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
            bool transparent = false;

            if ((paletteIndex & 0x0F) == 0)
            {
                paletteIndex = 0;
                transparent = true;
            }

            uint16_t bgr555 = palettePtr[paletteIndex];
            frameBuffer_.PushPixel({PixelSrc::BG2, bgr555, bgControl.flags_.bgPriority_, transparent}, i);
        }
    }
}

void PPU::RenderSprites()
{
    std::array<Pixel, 240> pixels;
    OamEntry const* oam = reinterpret_cast<OamEntry const*>(OAM_.data());

    for (int i = 0; i < 128; ++i)
    {
        OamEntry const& oamEntry = oam[i];

        // Check if this sprite is disabled
        if (!oamEntry.attribute0_.rotationOrScaling_ && oamEntry.attribute0_.doubleSizeOrDisable_)
        {
            continue;
        }

        if (oamEntry.attribute0_.rotationOrScaling_)
        {
            // Affine sprite, TODO
            continue;
        }

        // Determine the dimensions of this sprite in terms of pixels
        int height = 0;
        int width = 0;
        uint8_t pixelDimensions = (oamEntry.attribute0_.objShape_ << 2) | oamEntry.attribute1_.sharedFlags_.objSize_;

        switch (pixelDimensions)
        {
            // Square
            case 0b00'00:
                height = 8;
                width = 8;
                break;
            case 0b00'01:
                height = 16;
                width = 16;
                break;
            case 0b00'10:
                height = 32;
                width = 32;
                break;
            case 0b00'11:
                height = 64;
                width = 64;
                break;

            // Horizontal
            case 0b01'00:
                height = 8;
                width = 16;
                break;
            case 0b01'01:
                height = 8;
                width = 32;
                break;
            case 0b01'10:
                height = 16;
                width = 32;
                break;
            case 0b01'11:
                height = 32;
                width = 64;
                break;

            // Vertical
            case 0b10'00:
                height = 16;
                width = 8;
                break;
            case 0b10'01:
                height = 32;
                width = 8;
                break;
            case 0b10'10:
                height = 32;
                width = 16;
                break;
            case 0b10'11:
                height = 64;
                width = 32;
                break;

            // Illegal combination
            default:
                continue;
        }

        int y = oamEntry.attribute0_.yCoordinate_;
        int x = oamEntry.attribute1_.sharedFlags_.xCoordinate_;

        if (y >= 160)
        {
            y -= 256;
        }

        if (x & 0x0100)
        {
            x = (~0x01FF) | (x & 0x01FF);
        }

        // Check if this sprite exists on the current scanline
        if ((y > scanline_) || (scanline_ > (y + height - 1)))
        {
            continue;
        }

        if (oamEntry.attribute0_.objMode_ == 2)
        {
            // Window sprite, TODO
            continue;
        }
        else
        {
            if (lcdControl_.flags_.objCharacterVramMapping)
            {
                // One dimensional mapping
            }
            else
            {
                // Two dimensional mapping
                if (oamEntry.attribute0_.colorMode_)
                {
                    // 8bpp
                }
                else
                {
                    // 4bpp
                    Render2d4bppSprite(x, y, width, height, oamEntry, pixels);
                }
            }
        }
    }

    for (size_t i = 0; i < 240; ++i)
    {
        if (pixels[i].initialized_)
        {
            frameBuffer_.PushPixel(pixels[i], i);
        }
    }
}

void PPU::RenderRegularTiledBackgroundScanline(int bgIndex, BGCNT const& control, int xOffset, int yOffset)
{
    // Map size in pixels
    int const mapPixelWidth = (control.flags_.screenSize_ & 0x01) ? 512 : 256;
    int const mapPixelHeight = (control.flags_.screenSize_ & 0x02) ? 512 : 256;

    // Pixel coordinates
    int dotX = xOffset % mapPixelWidth;
    int const dotY = (scanline_ + yOffset) % mapPixelHeight;

    // Screen entry coordinates
    int mapX = dotX / 8;
    int mapY = dotY / 8;

    // Screenblock/Charblock addresses
    size_t screenBlockAddr = control.flags_.screenBaseBlock_ * SCREENBLOCK_SIZE;
    size_t const charBlockAddr = control.flags_.charBaseBlock_ * CHARBLOCK_SIZE;

    if (mapY > 31)
    {
        screenBlockAddr += (SCREENBLOCK_SIZE * ((mapPixelWidth == 256) ? 1 : 2));
        mapY %= 32;
    }

    // Memory pointers
    ScreenBlockEntry const* screenBlockEntryPtr = nullptr;
    uint16_t const* const palettePtr = reinterpret_cast<uint16_t const*>(paletteRAM_.data());

    // Status
    int totalPixelsDrawn = 0;
    int dot = 0;
    int const priority = control.flags_.bgPriority_;
    PixelSrc const src = static_cast<PixelSrc>(bgIndex + 1);

    while (totalPixelsDrawn < 240)
    {
        if (screenBlockEntryPtr == nullptr)
        {
            mapX = dotX / 8;
            ScreenBlock const* screenBlockPtr = reinterpret_cast<ScreenBlock const*>(&VRAM_.at(screenBlockAddr));

            if (mapX > 31)
            {
                ++screenBlockPtr;
                mapX %= 32;
            }

            screenBlockEntryPtr = &(screenBlockPtr->screenBlockEntry_[mapY][mapX]);
        }

        int tileX = dotX % 8;
        int tileY = dotY % 8;
        int pixelsToDraw = std::min(8 - tileX, 240 - totalPixelsDrawn);
        int pixelsDrawn = 0;

        if (screenBlockEntryPtr->verticalFlip_)
        {
            tileY ^= 7;
        }

        if (control.flags_.colorMode_)
        {
            // 8bpp
            TileData8bpp const* tilePtr = reinterpret_cast<TileData8bpp const*>(&VRAM_.at(charBlockAddr));
            bool flipped = screenBlockEntryPtr->horizontalFlip_;

            if (flipped)
            {
                tileX ^= 7;
            }

            while (pixelsDrawn < pixelsToDraw)
            {
                size_t paletteIndex = 0;

                if (screenBlockEntryPtr->tile_ < 512)
                {
                    paletteIndex = tilePtr[screenBlockEntryPtr->tile_].paletteIndex_[tileY][tileX];
                }

                uint16_t bgr555 = palettePtr[paletteIndex];
                bool transparent = (paletteIndex == 0);

                frameBuffer_.PushPixel({src, bgr555, priority, transparent}, dot);
                tileX += flipped ? -1 : 1;
                ++pixelsDrawn;
                ++dot;
            }
        }
        else
        {
            // 4bpp
            TileData4bpp const* tilePtr = reinterpret_cast<TileData4bpp const*>(&VRAM_.at(charBlockAddr));
            bool flipped = screenBlockEntryPtr->horizontalFlip_;

            if (flipped)
            {
                tileX ^= 7;
            }

            bool leftHalf = (tileX % 2) == 0;

            while (pixelsDrawn < pixelsToDraw)
            {
                size_t paletteIndex = screenBlockEntryPtr->palette_ << 4;
                auto paletteData = tilePtr[screenBlockEntryPtr->tile_].paletteIndex_[tileY][tileX / 2];
                paletteIndex |= leftHalf ? paletteData.leftNibble_ : paletteData.rightNibble_;
                bool transparent = false;

                if ((paletteIndex & 0x0F) == 0)
                {
                    transparent = true;
                    paletteIndex = 0;
                }

                uint16_t bgr555 = palettePtr[paletteIndex];
                frameBuffer_.PushPixel({src, bgr555, priority, transparent}, dot);
                tileX += flipped ? -1 : 1;
                ++pixelsDrawn;
                ++dot;
                leftHalf = !leftHalf;
            }
        }

        int nextDotX = (dotX + pixelsDrawn) % mapPixelWidth;

        if ((nextDotX / 256) != (dotX / 256))
        {
            screenBlockEntryPtr = nullptr;
        }
        else
        {
            ++screenBlockEntryPtr;
        }

        dotX = nextDotX;
        totalPixelsDrawn += pixelsDrawn;
    }
}

void PPU::Render2d4bppSprite(int x, int y, int width, int height, OamEntry const& oamEntry, std::array<Pixel, 240>& pixels)
{
    TwoDim4bppMap const* const tileMapPtr = reinterpret_cast<TwoDim4bppMap const*>(&VRAM_[OBJ_CHARBLOCK_ADDR]);
    uint16_t const* const palettePtr = reinterpret_cast<uint16_t const*>(&paletteRAM_[OBJ_PALETTE_ADDR]);
    TileData4bpp const* tileDataPtr = nullptr;

    int const widthInTiles = width / 8;
    int const heightInTiles = height / 8;

    int const leftEdge = std::max(0, x);
    int const rightEdge = std::min(239, x + width - 1);

    bool const verticalFlip = oamEntry.attribute1_.noRotationOrScaling_.verticalFlip_;
    bool const horizontalFlip = oamEntry.attribute1_.noRotationOrScaling_.horizontalFlip_;

    int const baseMapX = oamEntry.attribute2_.tile_ % 32;
    int const baseMapY = oamEntry.attribute2_.tile_ / 32;

    int mapX = 0;
    int const mapY = verticalFlip ?
        (baseMapY + (heightInTiles - ((scanline_ - y) / 8) - 1)) % 32 :
        (baseMapY + ((scanline_ - y) / 8)) % 32;

    int const tileY = verticalFlip ?
        ((scanline_ - y) % 8) ^ 7 :
        (scanline_ - y) % 8;

    int dot = leftEdge;
    int palette = oamEntry.attribute2_.palette_ << 4;
    int priority = oamEntry.attribute2_.priority_;

    while (dot <= rightEdge)
    {
        if (tileDataPtr == nullptr)
        {
            mapX = horizontalFlip ?
                ((baseMapX + (widthInTiles - ((dot - x) / 8) - 1)) % 32) :
                (baseMapX + ((dot - x) / 8)) % 32;

            tileDataPtr = &tileMapPtr->tileData_[mapY][mapX];
        }

        int tileX = (dot - x) % 8;
        int pixelsToDraw = std::min(8 - tileX, rightEdge - dot + 1);

        if (horizontalFlip)
        {
            tileX ^= 7;
        }

        bool leftHalf = (tileX % 2) == 0;

        while (pixelsToDraw > 0)
        {
            auto paletteData = tileDataPtr->paletteIndex_[tileY][tileX / 2];
            int paletteIndex = palette | (leftHalf ? paletteData.leftNibble_ : paletteData.rightNibble_);
            bool transparent = (paletteIndex & 0x0F) == 0;
            uint16_t bgr555 = palettePtr[paletteIndex];

            if (!pixels.at(dot).initialized_ ||
                (!transparent && pixels[dot].transparent_) ||
                (priority < pixels[dot].priority_))
            {
                pixels[dot] = Pixel(PixelSrc::OBJ,
                                    bgr555,
                                    priority,
                                    transparent,
                                    (oamEntry.attribute0_.objMode_ == 1));
            }

            --pixelsToDraw;
            ++dot;
            leftHalf = !leftHalf;
            tileX += (horizontalFlip ? -1 : 1);
        }

        if ((mapX < 31) && !horizontalFlip)
        {
            ++tileDataPtr;
        }
        else if ((mapX > 0) && horizontalFlip)
        {
            --tileDataPtr;
        }
        else
        {
            tileDataPtr = nullptr;
        }
    }
}
}
