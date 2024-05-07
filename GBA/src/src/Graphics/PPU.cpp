#include <Graphics/PPU.hpp>
#include <algorithm>
#include <cstdint>
#include <format>
#include <functional>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <Graphics/VramTypes.hpp>
#include <System/InterruptManager.hpp>
#include <System/MemoryMap.hpp>
#include <System/Scheduler.hpp>
#include <System/Utilities.hpp>

namespace
{
int WrapModulo(int dividend, int divisor)
{
    return ((dividend % divisor) + divisor) % divisor;
}
}

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
    window0EnabledOnScanline_ = false;
    window1EnabledOnScanline_ = false;
    lcdRegisters_.fill(0);

    Scheduler.RegisterEvent(EventType::VDraw, std::bind(&VDraw, this, std::placeholders::_1));
}

std::tuple<uint32_t, int, bool> PPU::ReadLcdReg(uint32_t addr, AccessSize alignment)
{
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
    if ((addr >= DISPSTAT_ADDR) && (addr < VCOUNT_ADDR))
    {
        // Write to DISPSTAT; not all bits are writable. If writing a word, the next register is VCOUNT so don't write that either.
        static constexpr uint16_t DISPSTAT_WRITABLE_MASK = 0b1111'1111'1011'1000;

        switch (alignment)
        {
            case AccessSize::BYTE:
            {
                if (addr == DISPSTAT_ADDR)
                {
                    lcdStatus_.halfword_ = (lcdStatus_.halfword_ & 0xFF00) |
                                           (((lcdStatus_.halfword_ & ~DISPSTAT_WRITABLE_MASK) | (value & DISPSTAT_WRITABLE_MASK)) & MAX_U8);
                }
                else
                {
                    lcdStatus_.flags_.vCountSetting = (value & MAX_U8);
                }
                break;
            }
            case AccessSize::HALFWORD:
            case AccessSize::WORD:
                lcdStatus_.halfword_ = (lcdStatus_.halfword_ & ~DISPSTAT_WRITABLE_MASK) | (value & DISPSTAT_WRITABLE_MASK);
                break;
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

    if ((index >= 0x28) && (index <= 0x3F))
    {
        if (index <= 0x2B)
        {
            bg2RefX_ = SignExtend32(*reinterpret_cast<uint32_t*>(&lcdRegisters_[0x28]), 27);
        }
        else if (index <= 0x2F)
        {
            bg2RefY_ = SignExtend32(*reinterpret_cast<uint32_t*>(&lcdRegisters_[0x2C]), 27);
        }
        else if (index <= 0x3B)
        {
            bg3RefX_ = SignExtend32(*reinterpret_cast<uint32_t*>(&lcdRegisters_[0x38]), 27);
        }
        else
        {
            bg3RefY_ = SignExtend32(*reinterpret_cast<uint32_t*>(&lcdRegisters_[0x3C]), 27);
        }
    }


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

    SetNonObjWindowEnabled();

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
        bool windowEnabled = (lcdControl_.halfword_ & 0xE000) != 0;

        if (lcdControl_.flags_.forceBlank)
        {
            backdrop = 0xFFFF;
        }
        else
        {
            if (windowEnabled)
            {
                WININ const& winin = *reinterpret_cast<WININ const*>(&lcdRegisters_[0x48]);
                WINOUT const& winout = *reinterpret_cast<WINOUT const*>(&lcdRegisters_[0x4A]);

                #pragma GCC diagnostic push
                #pragma GCC diagnostic ignored "-Wnarrowing"
                WindowSettings outOfWindow = {
                    {winout.outsideBg0Enabled_, winout.outsideBg1Enabled_, winout.outsideBg2Enabled_, winout.outsideBg3Enabled_},
                    winout.outsideObjEnabled_,
                    winout.outsideSpecialEffect_
                };
                #pragma GCC diagnostic pop

                frameBuffer_.InitializeWindow(outOfWindow);

                if (lcdControl_.flags_.screenDisplayObj && lcdControl_.flags_.objWindowDisplay)
                {
                    #pragma GCC diagnostic push
                    #pragma GCC diagnostic ignored "-Wnarrowing"
                    WindowSettings objWindow = {
                        {winout.objWinBg0Enabled_, winout.objWinBg1Enabled_, winout.objWinBg2Enabled_, winout.objWinBg3Enabled_},
                        winout.objWinObjEnabled_,
                        winout.objWinSpecialEffect_
                    };
                    #pragma GCC diagnostic pop

                    EvaluateOAM(&objWindow);
                }

                if (lcdControl_.flags_.window1Display)
                {
                    #pragma GCC diagnostic push
                    #pragma GCC diagnostic ignored "-Wnarrowing"
                    WindowSettings window1 = {
                        {winin.win1Bg0Enabled_, winin.win1Bg1Enabled_, winin.win1Bg2Enabled_, winin.win1Bg3Enabled_},
                        winin.win1ObjEnabled_,
                        winin.win1SpecialEffect_
                    };
                    #pragma GCC diagnostic pop

                    uint8_t x1 = lcdRegisters_[0x43];
                    uint8_t x2 = lcdRegisters_[0x42];

                    if (window1EnabledOnScanline_)
                    {
                        ConfigureNonObjWindow(x1, x2, window1);
                    }
                }

                if (lcdControl_.flags_.window0Display)
                {
                    #pragma GCC diagnostic push
                    #pragma GCC diagnostic ignored "-Wnarrowing"
                    WindowSettings window0 = {
                        {winin.win0Bg0Enabled_, winin.win0Bg1Enabled_, winin.win0Bg2Enabled_, winin.win0Bg3Enabled_},
                        winin.win0ObjEnabled_,
                        winin.win0SpecialEffect_
                    };
                    #pragma GCC diagnostic pop

                    uint8_t x1 = lcdRegisters_[0x41];
                    uint8_t x2 = lcdRegisters_[0x40];

                    if (window0EnabledOnScanline_)
                    {
                        ConfigureNonObjWindow(x1, x2, window0);
                    }
                }
            }
            else
            {
                WindowSettings allEnabled = {
                    {true, true, true, true},
                    true,
                    true
                };

                frameBuffer_.InitializeWindow(allEnabled);
            }

            if (lcdControl_.flags_.screenDisplayObj)
            {
                EvaluateOAM();
            }

            if (lcdControl_.flags_.screenDisplayObj)
            {
                frameBuffer_.PushSpritePixels();
            }

            switch (lcdControl_.flags_.bgMode)
            {
                case 0:
                    RenderMode0Scanline();
                    break;
                case 1:
                    RenderMode1Scanline();
                    break;
                case 2:
                    RenderMode2Scanline();
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

        frameBuffer_.RenderScanline(backdrop, windowEnabled);
        IncrementAffineBackgroundReferencePoints();
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
        Scheduler.ScheduleEvent(EventType::RefreshScreen, SCHEDULE_NOW);
        frameBuffer_.ResetFrameIndex();

        if (lcdStatus_.flags_.vBlankIrqEnable)
        {
            InterruptMgr.RequestInterrupt(InterruptType::LCD_VBLANK);
        }

        bg2RefX_ = SignExtend32(*reinterpret_cast<uint32_t*>(&lcdRegisters_[0x28]), 27);
        bg2RefY_ = SignExtend32(*reinterpret_cast<uint32_t*>(&lcdRegisters_[0x2C]), 27);
        bg3RefX_ = SignExtend32(*reinterpret_cast<uint32_t*>(&lcdRegisters_[0x38]), 27);
        bg3RefY_ = SignExtend32(*reinterpret_cast<uint32_t*>(&lcdRegisters_[0x3C]), 27);
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

    SetNonObjWindowEnabled();

    // Event Scheduling
    Scheduler.ScheduleEvent(EventType::HBlank, (960 - extraCycles) + 46);
}

void PPU::SetNonObjWindowEnabled()
{
    // Window 1 scanline in range check
    uint8_t y1 = lcdRegisters_[0x47];  // Top
    uint8_t y2 = lcdRegisters_[0x46];  // Bottom

    if (scanline_ == y1)
    {
        window1EnabledOnScanline_ = true;
    }

    if (scanline_ == y2)
    {
        window1EnabledOnScanline_ = false;
    }

    // Window 0 scanline in range check
    y1 = lcdRegisters_[0x45];  // Top
    y2 = lcdRegisters_[0x44];  // Bottom

    if (scanline_ == y1)
    {
        window0EnabledOnScanline_ = true;
    }

    if (scanline_ == y2)
    {
        window0EnabledOnScanline_ = false;
    }
}

void PPU::ConfigureNonObjWindow(uint8_t leftEdge, uint8_t rightEdge, WindowSettings settings)
{
    if (rightEdge > 240)
    {
        rightEdge = 240;
    }

    if (leftEdge <= rightEdge)
    {
        for (int dot = leftEdge; dot < rightEdge; ++dot)
        {
            frameBuffer_.GetWindowSettings(dot) = settings;
        }
    }
    else
    {
        for (int dot = 0; dot < rightEdge; ++dot)
        {
            frameBuffer_.GetWindowSettings(dot) = settings;
        }

        for (int dot = leftEdge; dot < 240; ++dot)
        {
            frameBuffer_.GetWindowSettings(dot) = settings;
        }
    }
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

void PPU::RenderMode1Scanline()
{
    for (uint16_t i = 0; i < 2; ++i)
    {
        if (lcdControl_.halfword_ & (0x100 << i))
        {
            BGCNT const& bgControl = *reinterpret_cast<BGCNT*>(&lcdRegisters_[0x08 + (2 * i)]);
            uint16_t xOffset = *reinterpret_cast<uint16_t*>(&lcdRegisters_[0x10 + (4 * i)]) & 0x01FF;
            uint16_t yOffset = *reinterpret_cast<uint16_t*>(&lcdRegisters_[0x12 + (4 * i)]) & 0x01FF;
            RenderRegularTiledBackgroundScanline(i, bgControl, xOffset, yOffset);
        }
    }

    if (lcdControl_.flags_.screenDisplayBg2)
    {
        BGCNT const& bgControl = *reinterpret_cast<BGCNT*>(&lcdRegisters_[0x0C]);
        int16_t pa = *reinterpret_cast<int16_t*>(&lcdRegisters_[0x20]);
        int16_t pc = *reinterpret_cast<int16_t*>(&lcdRegisters_[0x24]);
        RenderAffineTiledBackgroundScanline(2, bgControl, bg2RefX_, bg2RefY_, pa, pc);
    }
}

void PPU::RenderMode2Scanline()
{
    if (lcdControl_.flags_.screenDisplayBg2)
    {
        BGCNT const& bgControl = *reinterpret_cast<BGCNT*>(&lcdRegisters_[0x0C]);
        int16_t pa = *reinterpret_cast<int16_t*>(&lcdRegisters_[0x20]);
        int16_t pc = *reinterpret_cast<int16_t*>(&lcdRegisters_[0x24]);
        RenderAffineTiledBackgroundScanline(2, bgControl, bg2RefX_, bg2RefY_, pa, pc);
    }

    if (lcdControl_.flags_.screenDisplayBg3)
    {
        BGCNT const& bgControl = *reinterpret_cast<BGCNT*>(&lcdRegisters_[0x0E]);
        int16_t pa = *reinterpret_cast<int16_t*>(&lcdRegisters_[0x30]);
        int16_t pc = *reinterpret_cast<int16_t*>(&lcdRegisters_[0x34]);
        RenderAffineTiledBackgroundScanline(3, bgControl, bg3RefX_, bg3RefY_, pa, pc);
    }
}

void PPU::RenderMode3Scanline()
{
    BGCNT const& bgControl = *reinterpret_cast<BGCNT*>(&lcdRegisters_[0x0C]);
    size_t vramIndex = scanline_ * 480;
    uint16_t const* vramPtr = reinterpret_cast<uint16_t const*>(&VRAM_.at(vramIndex));

    if (lcdControl_.flags_.screenDisplayBg2)
    {
        for (int dot = 0; dot < 240; ++dot)
        {
            if (frameBuffer_.GetWindowSettings(dot).bgEnabled_[2])
            {
                frameBuffer_.PushPixel({PixelSrc::BG2, *vramPtr, bgControl.flags_.bgPriority_, false}, dot);
            }

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
        for (int dot = 0; dot < 240; ++dot)
        {
            uint8_t paletteIndex = VRAM_.at(vramIndex++);
            bool transparent = false;

            if ((paletteIndex & 0x0F) == 0)
            {
                paletteIndex = 0;
                transparent = true;
            }

            uint16_t bgr555 = palettePtr[paletteIndex];

            if (frameBuffer_.GetWindowSettings(dot).bgEnabled_[2])
            {
                frameBuffer_.PushPixel({PixelSrc::BG2, bgr555, bgControl.flags_.bgPriority_, transparent}, dot);
            }
        }
    }
}

void PPU::RenderRegularTiledBackgroundScanline(int bgIndex, BGCNT const& control, int xOffset, int yOffset)
{
    int const width = (control.flags_.screenSize_ & 0b01) ? 512 : 256;
    int const height = (control.flags_.screenSize_ & 0b10) ? 512 : 256;

    int const x = xOffset % width;
    int const y = (scanline_ + yOffset) % height;

    if (control.flags_.colorMode_)
    {
        RenderRegular8bppBackground(bgIndex, control, x, y, width);
    }
    else
    {
        RenderRegular4bppBackground(bgIndex, control, x, y, width);
    }
}

void PPU::RenderRegular4bppBackground(int bgIndex, BGCNT const& control, int x, int y, int width)
{
    ScreenBlock const* screenBlockPtr =
        reinterpret_cast<ScreenBlock const*>(&VRAM_[control.flags_.screenBaseBlock_ * SCREENBLOCK_SIZE]);

    if (y > 255)
    {
        ++screenBlockPtr;

        if (width == 512)
        {
            ++screenBlockPtr;
        }
    }

    int const mapY = (y / 8) % 32;
    TileData4bpp const* baseTilePtr = reinterpret_cast<TileData4bpp const*>(&VRAM_[control.flags_.charBaseBlock_ * CHARBLOCK_SIZE]);
    ScreenBlockEntry const* screenBlockEntryPtr = nullptr;
    uint16_t const* palettePtr = reinterpret_cast<uint16_t const*>(&paletteRAM_[0]);

    // Control data
    PixelSrc const src = static_cast<PixelSrc>(bgIndex + 1);
    int const priority = control.flags_.bgPriority_;

    // Screen block entry
    TileData4bpp const* tileData = nullptr;
    bool horizontalFlip = false;
    bool verticalFlip = false;
    size_t palette = 0;
    int tileY = 0;

    for (int dot = 0; dot < LCD_WIDTH; ++dot)
    {
        if (frameBuffer_.GetWindowSettings(dot).bgEnabled_[bgIndex])
        {
            if (screenBlockEntryPtr == nullptr)
            {
                int mapX = x / 8;
                size_t screenBlockIndex = (mapX > 31) ? 1 : 0;
                screenBlockEntryPtr = &screenBlockPtr[screenBlockIndex].screenBlockEntry_[mapY][mapX % 32];

                tileData = &baseTilePtr[screenBlockEntryPtr->tile_];
                horizontalFlip = screenBlockEntryPtr->horizontalFlip_;
                verticalFlip = screenBlockEntryPtr->verticalFlip_;
                palette = (screenBlockEntryPtr->palette_ << 4);
                tileY = verticalFlip ? ((y % 8) ^ 7) : (y % 8);
            }

            int tileX = horizontalFlip ? ((x % 8) ^ 7) : (x % 8);
            bool left = (tileX % 2) == 0;
            auto tileNibbles = tileData->paletteIndex_[tileY][tileX / 2];
            size_t paletteIndex = palette | (left ? tileNibbles.leftNibble_ : tileNibbles.rightNibble_);
            bool transparent = (paletteIndex & 0x0F) == 0;

            if (transparent)
            {
                paletteIndex = 0;
            }

            uint16_t bgr555 = palettePtr[paletteIndex];
            frameBuffer_.PushPixel({src, bgr555, priority, transparent}, dot);
        }

        x = (x + 1) % width;

        if ((x % 8) == 0)
        {
            screenBlockEntryPtr = nullptr;
        }
    }
}

void PPU::RenderRegular8bppBackground(int bgIndex, BGCNT const& control, int x, int y, int width)
{
    ScreenBlock const* screenBlockPtr =
        reinterpret_cast<ScreenBlock const*>(&VRAM_[control.flags_.screenBaseBlock_ * SCREENBLOCK_SIZE]);

    if (y > 255)
    {
        ++screenBlockPtr;

        if (width == 512)
        {
            ++screenBlockPtr;
        }
    }

    int const mapY = (y / 8) % 32;
    size_t const charBlockAddr = control.flags_.charBaseBlock_ * CHARBLOCK_SIZE;
    TileData8bpp const* baseTilePtr = reinterpret_cast<TileData8bpp const*>(&VRAM_[charBlockAddr]);
    ScreenBlockEntry const* screenBlockEntryPtr = nullptr;
    uint16_t const* palettePtr = reinterpret_cast<uint16_t const*>(&paletteRAM_[0]);

    // Control data
    PixelSrc const src = static_cast<PixelSrc>(bgIndex + 1);
    int const priority = control.flags_.bgPriority_;

    // Screen block entry
    TileData8bpp const* tileData = nullptr;
    bool horizontalFlip = false;
    bool verticalFlip = false;
    int tileY = 0;
    bool outOfRange = false;

    for (int dot = 0; dot < LCD_WIDTH; ++dot)
    {
        if (frameBuffer_.GetWindowSettings(dot).bgEnabled_[bgIndex])
        {
            if (screenBlockEntryPtr == nullptr)
            {
                int mapX = x / 8;
                size_t screenBlockIndex = (mapX > 31) ? 1 : 0;
                screenBlockEntryPtr = &screenBlockPtr[screenBlockIndex].screenBlockEntry_[mapY][mapX % 32];
                outOfRange = (charBlockAddr + (screenBlockEntryPtr->tile_ * sizeof(TileData8bpp))) >= 0x0001'0000;

                if (!outOfRange)
                {
                    tileData = &baseTilePtr[screenBlockEntryPtr->tile_];
                    horizontalFlip = screenBlockEntryPtr->horizontalFlip_;
                    verticalFlip = screenBlockEntryPtr->verticalFlip_;
                    tileY = verticalFlip ? ((y % 8) ^ 7) : (y % 8);
                }
            }

            if (!outOfRange)
            {
                int tileX = horizontalFlip ? ((x % 8) ^ 7) : (x % 8);
                size_t paletteIndex = tileData->paletteIndex_[tileY][tileX];
                bool transparent = (paletteIndex == 0);
                uint16_t bgr555 = palettePtr[paletteIndex];
                frameBuffer_.PushPixel({src, bgr555, priority, transparent}, dot);
            }
        }

        x = (x + 1) % width;

        if ((x % 8) == 0)
        {
            screenBlockEntryPtr = nullptr;
        }
    }
}

void PPU::RenderAffineTiledBackgroundScanline(int bgIndex, BGCNT const& control, int32_t dx, int32_t dy, int16_t pa, int16_t pc)
{
    // Map size
    int mapSizeInTiles;

    switch (control.flags_.screenSize_)
    {
        case 0:
            mapSizeInTiles = 16;
            break;
        case 1:
            mapSizeInTiles = 32;
            break;
        case 2:
            mapSizeInTiles = 64;
            break;
        case 3:
            mapSizeInTiles = 128;
            break;
    }

    int const mapSizeInPixels = mapSizeInTiles * 8;

    // Initialize affine position
    int32_t affineX = dx;
    int32_t affineY = dy;

    // Control fields
    bool const wrapOnOverflow = control.flags_.overflowMode_;
    int const priority = control.flags_.bgPriority_;
    PixelSrc const src = static_cast<PixelSrc>(bgIndex + 1);

    // VRAM pointers
    uint8_t const* screenBlockPtr = &VRAM_[control.flags_.screenBaseBlock_ * SCREENBLOCK_SIZE];
    TileData8bpp const* tilePtr = reinterpret_cast<TileData8bpp const*>(&VRAM_[control.flags_.charBaseBlock_ * CHARBLOCK_SIZE]);
    uint16_t const* palettePtr = reinterpret_cast<uint16_t const*>(paletteRAM_.data());

    for (int dot = 0; dot < LCD_WIDTH; ++dot)
    {
        if (frameBuffer_.GetWindowSettings(dot).bgEnabled_[bgIndex])
        {
            int32_t screenX = affineX >> 8;
            int32_t screenY = affineY >> 8;
            size_t paletteIndex = 0;
            bool transparent = true;
            bool calculateTile = wrapOnOverflow ||
                                 ((screenX >= 0) && (screenX < mapSizeInPixels) && (screenY >= 0) && (screenY < mapSizeInPixels));

            if (calculateTile)
            {
                screenX = WrapModulo(screenX, mapSizeInPixels);
                screenY = WrapModulo(screenY, mapSizeInPixels);
                int mapX = screenX / 8;
                int mapY = screenY / 8;
                int tileX = screenX % 8;
                int tileY = screenY % 8;
                size_t tileIndex = screenBlockPtr[mapX + (mapY * mapSizeInTiles)];
                paletteIndex = tilePtr[tileIndex].paletteIndex_[tileY][tileX];
                transparent = (paletteIndex == 0);
            }

            uint16_t bgr555 = palettePtr[paletteIndex];
            frameBuffer_.PushPixel({src, bgr555, priority, transparent}, dot);
        }

        affineX += pa;
        affineY += pc;
    }
}

void PPU::EvaluateOAM(WindowSettings* windowSettingsPtr)
{
    frameBuffer_.ClearSpritePixels();
    OamEntry const* oam = reinterpret_cast<OamEntry const*>(OAM_.data());
    bool const evaluateWindowSprites = windowSettingsPtr != nullptr;

    for (int i = 0; i < 128; ++i)
    {
        OamEntry const& oamEntry = oam[i];

        // Skip window sprites when evaluating visible sprites, and vice versa
        if ((evaluateWindowSprites && (oamEntry.attribute0_.objMode_ != 2)) ||
            (!evaluateWindowSprites && (oamEntry.attribute0_.objMode_ == 2)))
        {
            continue;
        }

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

        if (lcdControl_.flags_.objCharacterVramMapping)
        {
            // One dimensional mapping
            if (oamEntry.attribute0_.colorMode_)
            {
                // 8bpp
                continue;
            }
            else
            {
                // 4bpp
                Render1d4bppSprite(x, y, width, height, oamEntry, windowSettingsPtr);
            }
        }
        else
        {
            // Two dimensional mapping
            if (oamEntry.attribute0_.colorMode_)
            {
                // 8bpp
                continue;
            }
            else
            {
                // 4bpp
                Render2d4bppSprite(x, y, width, height, oamEntry, windowSettingsPtr);
            }
        }
    }
}

void PPU::Render1d4bppSprite(int x, int y, int width, int height, OamEntry const& oamEntry, WindowSettings* windowSettingsPtr)
{
    int const widthInTiles = width / 8;
    int const heightInTiles = height / 8;

    TileData4bpp const* const tileMapPtr = reinterpret_cast<TileData4bpp const*>(&VRAM_[OBJ_CHARBLOCK_ADDR]);
    uint16_t const* const palettePtr = reinterpret_cast<uint16_t const*>(&paletteRAM_[OBJ_PALETTE_ADDR]);

    int const leftEdge = std::max(0, x);
    int const rightEdge = std::min(239, x + width - 1);

    bool const verticalFlip = oamEntry.attribute1_.noRotationOrScaling_.verticalFlip_;
    bool const horizontalFlip = oamEntry.attribute1_.noRotationOrScaling_.horizontalFlip_;

    int const baseTileIndex = verticalFlip ?
        (oamEntry.attribute2_.tile_ + ((heightInTiles - ((scanline_ - y) / 8) - 1) * widthInTiles)) % 1024 :
        (oamEntry.attribute2_.tile_ + (((scanline_ - y) / 8) * widthInTiles)) % 1024;

    int const tileY = verticalFlip ?
        ((scanline_ - y) % 8) ^ 7 :
        (scanline_ - y) % 8;

    int dot = leftEdge;
    int const palette = oamEntry.attribute2_.palette_ << 4;
    int const priority = oamEntry.attribute2_.priority_;

    while (dot <= rightEdge)
    {
        int tileOffset = horizontalFlip ?
            widthInTiles - ((dot - x) / 8) - 1 :
            (dot - x) / 8;

        TileData4bpp const* const tileDataPtr = &tileMapPtr[(baseTileIndex + tileOffset) % 1024];

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

            if (windowSettingsPtr == nullptr)
            {
                // Visible Sprite
                if (frameBuffer_.GetWindowSettings(dot).objEnabled_ &&
                    ((!frameBuffer_.GetSpritePixel(dot).initialized_ ||
                    (!transparent && frameBuffer_.GetSpritePixel(dot).transparent_) ||
                    (priority < frameBuffer_.GetSpritePixel(dot).priority_))))
                {
                    frameBuffer_.GetSpritePixel(dot) = Pixel(PixelSrc::OBJ,
                                                             bgr555,
                                                             priority,
                                                             transparent,
                                                             (oamEntry.attribute0_.objMode_ == 1));
                }
            }
            else if (!transparent)
            {
                // Opaque OBJ window sprite pixel
                frameBuffer_.GetWindowSettings(dot) = *windowSettingsPtr;
            }

            --pixelsToDraw;
            ++dot;
            leftHalf = !leftHalf;
            tileX += (horizontalFlip ? -1 : 1);
        }
    }
}

void PPU::Render2d4bppSprite(int x, int y, int width, int height, OamEntry const& oamEntry, WindowSettings* windowSettingsPtr)
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
    int const palette = oamEntry.attribute2_.palette_ << 4;
    int const priority = oamEntry.attribute2_.priority_;

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

            if (windowSettingsPtr == nullptr)
            {
                // Visible Sprite
                if (frameBuffer_.GetWindowSettings(dot).objEnabled_ &&
                    ((!frameBuffer_.GetSpritePixel(dot).initialized_ ||
                    (!transparent && frameBuffer_.GetSpritePixel(dot).transparent_) ||
                    (priority < frameBuffer_.GetSpritePixel(dot).priority_))))
                {
                    frameBuffer_.GetSpritePixel(dot) = Pixel(PixelSrc::OBJ,
                                                             bgr555,
                                                             priority,
                                                             transparent,
                                                             (oamEntry.attribute0_.objMode_ == 1));
                }
            }
            else if (!transparent)
            {
                // Opaque OBJ window sprite pixel
                frameBuffer_.GetWindowSettings(dot) = *windowSettingsPtr;
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

void PPU::IncrementAffineBackgroundReferencePoints()
{
    bg2RefX_ += *reinterpret_cast<int16_t*>(&lcdRegisters_[0x22]);  // PB
    bg2RefY_ += *reinterpret_cast<int16_t*>(&lcdRegisters_[0x26]);  // PD
    bg3RefX_ += *reinterpret_cast<int16_t*>(&lcdRegisters_[0x32]);  // PB
    bg3RefY_ += *reinterpret_cast<int16_t*>(&lcdRegisters_[0x36]);  // PD
}
}
