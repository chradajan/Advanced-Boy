#include <Graphics/PPU.hpp>
#include <algorithm>
#include <cstdint>
#include <format>
#include <functional>
#include <stdexcept>
#include <utility>
#include <Graphics/VramTypes.hpp>
#include <System/MemoryMap.hpp>
#include <System/EventScheduler.hpp>
#include <System/SystemControl.hpp>
#include <Utilities/MemoryUtilities.hpp>

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
static_assert(sizeof(AffineObjMatrix) == 32);
static_assert(sizeof(TwoDim4bppMap) == 2 * CHARBLOCK_SIZE);
static_assert(sizeof(TwoDim8bppMap) == 2 * CHARBLOCK_SIZE);

PPU::PPU() :
    lcdRegisters_(),
    dispcnt_(*reinterpret_cast<DISPCNT*>(&lcdRegisters_[0])),
    dispstat_(*reinterpret_cast<DISPSTAT*>(&lcdRegisters_[4])),
    vcount_(*reinterpret_cast<VCOUNT*>(&lcdRegisters_[6]))
{
    Scheduler.RegisterEvent(EventType::VDraw, std::bind(&VDraw, this, std::placeholders::_1), true);
}

void PPU::Reset()
{
    scanline_ = 0;
    window0EnabledOnScanline_ = false;
    window1EnabledOnScanline_ = false;
    lcdRegisters_.fill(0);
    frameCounter_ = 0;
    frameBuffer_.Reset();
}

std::pair<uint32_t, int> PPU::ReadPRAM(uint32_t addr, AccessSize alignment)
{
    if (addr > PALETTE_RAM_ADDR_MAX)
    {
        addr = PALETTE_RAM_ADDR_MIN + (addr % PRAM_.size());
    }

    size_t index = addr - PALETTE_RAM_ADDR_MIN;
    uint8_t* bytePtr = &PRAM_.at(index);
    uint32_t value = ReadPointer(bytePtr, alignment);
    int cycles = (alignment == AccessSize::WORD) ? 2 : 1;
    return {value, cycles};
}

int PPU::WritePRAM(uint32_t addr, uint32_t value, AccessSize alignment)
{
    if (addr > PALETTE_RAM_ADDR_MAX)
    {
        addr = PALETTE_RAM_ADDR_MIN + (addr % PRAM_.size());
    }

    int cycles = (alignment == AccessSize::WORD) ? 2 : 1;

    if (alignment == AccessSize::BYTE)
    {
        alignment = AccessSize::HALFWORD;
        value = ((value & MAX_U8) << 8) | (value & MAX_U8);
    }

    size_t index = addr - PALETTE_RAM_ADDR_MIN;
    uint8_t* bytePtr = &PRAM_.at(index);
    WritePointer(bytePtr, value, alignment);
    return cycles;
}

std::pair<uint32_t, int> PPU::ReadVRAM(uint32_t addr, AccessSize alignment)
{
    if (addr > VRAM_ADDR_MAX)
    {
        uint32_t adjustedAddr = VRAM_ADDR_MIN + (addr % (128 * KiB));

        if (adjustedAddr > VRAM_ADDR_MAX)
        {
            adjustedAddr -= (32 * KiB);
        }

        addr = adjustedAddr;
    }

    size_t index = addr - VRAM_ADDR_MIN;
    uint8_t* bytePtr = &VRAM_.at(index);
    uint32_t value = ReadPointer(bytePtr, alignment);
    int cycles = (alignment == AccessSize::WORD) ? 2 : 1;
    return {value, cycles};
}

int PPU::WriteVRAM(uint32_t addr, uint32_t value, AccessSize alignment)
{
    if (addr > VRAM_ADDR_MAX)
    {
        uint32_t adjustedAddr = VRAM_ADDR_MIN + (addr % (128 * KiB));

        if (adjustedAddr > VRAM_ADDR_MAX)
        {
            adjustedAddr -= (32 * KiB);
        }

        addr = adjustedAddr;
    }

    if (alignment == AccessSize::BYTE)
    {
        if (((dispcnt_.bgMode <= 2) && (addr >= 0x0601'0000)) || ((dispcnt_.bgMode > 2) && (addr >= 0x0601'4000)))
        {
            return 1;
        }

        alignment = AccessSize::HALFWORD;
        value = ((value & MAX_U8) << 8) | (value & MAX_U8);
    }

    size_t index = addr - VRAM_ADDR_MIN;
    uint8_t* bytePtr = &(VRAM_.at(index));
    WritePointer(bytePtr, value, alignment);
    return (alignment == AccessSize::WORD) ? 2 : 1;
}

std::pair<uint32_t, int> PPU::ReadOAM(uint32_t addr, AccessSize alignment)
{
    if (addr > OAM_ADDR_MAX)
    {
        addr = OAM_ADDR_MIN + (addr % OAM_.size());
    }

    size_t index = addr - OAM_ADDR_MIN;
    uint8_t* bytePtr = &OAM_.at(index);
    uint32_t value = ReadPointer(bytePtr, alignment);
    return {value, 1};
}

int PPU::WriteOAM(uint32_t addr, uint32_t value, AccessSize alignment)
{
    if (alignment == AccessSize::BYTE)
    {
        return 1;
    }

    if (addr > OAM_ADDR_MAX)
    {
        addr = OAM_ADDR_MIN + (addr % OAM_.size());
    }

    size_t index = addr - OAM_ADDR_MIN;
    uint8_t* bytePtr = &(OAM_.at(index));
    WritePointer(bytePtr, value, alignment);
    return 1;
}

std::pair<uint32_t, bool> PPU::ReadReg(uint32_t addr, AccessSize alignment)
{
    if (((0x0400'0010 <= addr) && (addr < 0x0400'0048)) ||
        ((0x0400'004C <= addr) && (addr < 0x0400'0050)) ||
        ((0x0400'0054 <= addr) && (addr < 0x0400'0058)))
    {
        // Write only regions.
        return {0, true};
    }

    size_t index = addr - LCD_IO_ADDR_MIN;
    uint8_t* bytePtr = &lcdRegisters_.at(index);
    uint32_t value = ReadPointer(bytePtr, alignment);
    return {value, false};
}

void PPU::WriteReg(uint32_t addr, uint32_t value, AccessSize alignment)
{
    if ((0x0400'0004 <= addr) && (addr < 0x0400'0008))
    {
        WriteDispstatVcount(addr, value, alignment);
        return;
    }

    size_t index = addr - LCD_IO_ADDR_MIN;
    uint8_t* bytePtr = &lcdRegisters_.at(index);
    WritePointer(bytePtr, value, alignment);

    if ((0x0400'0028 <= addr) && (addr < 0x0400'002C))
    {
        bg2RefX_ = SignExtend32(*reinterpret_cast<uint32_t*>(&lcdRegisters_[0x28]), 27);
    }
    else if ((0x0400'002C <= addr) && (addr < 0x0400'0030))
    {
        bg2RefY_ = SignExtend32(*reinterpret_cast<uint32_t*>(&lcdRegisters_[0x2C]), 27);
    }
    else if ((0x0400'0038 <= addr) && (addr < 0x0400'003C))
    {
        bg3RefX_ = SignExtend32(*reinterpret_cast<uint32_t*>(&lcdRegisters_[0x38]), 27);
    }
    else if ((0x0400'003C <= addr) && (addr < 0x0400'0040))
    {
        bg3RefY_ = SignExtend32(*reinterpret_cast<uint32_t*>(&lcdRegisters_[0x3C]), 27);
    }
}

void PPU::HBlank(int extraCycles)
{
    // HBlank register updates
    dispstat_.hBlank = 1;

    if (dispstat_.hBlankIrqEnable)
    {
        SystemController.RequestInterrupt(InterruptType::LCD_HBLANK);
    }

    // Event scheduling
    int cyclesUntilNextEvent = 226 - extraCycles;
    EventType nextEvent = ((scanline_ < 159) || (scanline_ == 227)) ? EventType::VDraw : EventType::VBlank;
    Scheduler.ScheduleEvent(nextEvent, cyclesUntilNextEvent);

    // Draw scanline if not in VBlank
    if (scanline_ < 160)
    {
        uint16_t backdrop = *reinterpret_cast<uint16_t const*>(&PRAM_[0]);
        bool windowEnabled = (dispcnt_.value & 0xE000) != 0;
        bool forceBlank = dispcnt_.forceBlank;

        if (!forceBlank)
        {
            if (windowEnabled)
            {
                WININ const& winin = *reinterpret_cast<WININ const*>(&lcdRegisters_[0x48]);
                WINOUT const& winout = *reinterpret_cast<WINOUT const*>(&lcdRegisters_[0x4A]);

                #pragma GCC diagnostic push
                #pragma GCC diagnostic ignored "-Wnarrowing"
                WindowSettings outOfWindow = {
                    {winout.outsideBg0Enabled, winout.outsideBg1Enabled, winout.outsideBg2Enabled, winout.outsideBg3Enabled},
                    winout.outsideObjEnabled,
                    winout.outsideSpecialEffect
                };
                #pragma GCC diagnostic pop

                frameBuffer_.InitializeWindow(outOfWindow);

                if (dispcnt_.screenDisplayObj && dispcnt_.objWindowDisplay)
                {
                    #pragma GCC diagnostic push
                    #pragma GCC diagnostic ignored "-Wnarrowing"
                    WindowSettings objWindow = {
                        {winout.objWinBg0Enabled, winout.objWinBg1Enabled, winout.objWinBg2Enabled, winout.objWinBg3Enabled},
                        winout.objWinObjEnabled,
                        winout.objWinSpecialEffect
                    };
                    #pragma GCC diagnostic pop

                    EvaluateOAM(&objWindow);
                }

                if (dispcnt_.window1Display)
                {
                    #pragma GCC diagnostic push
                    #pragma GCC diagnostic ignored "-Wnarrowing"
                    WindowSettings window1 = {
                        {winin.win1Bg0Enabled, winin.win1Bg1Enabled, winin.win1Bg2Enabled, winin.win1Bg3Enabled},
                        winin.win1ObjEnabled,
                        winin.win1SpecialEffect
                    };
                    #pragma GCC diagnostic pop

                    uint8_t x1 = lcdRegisters_[0x43];
                    uint8_t x2 = lcdRegisters_[0x42];

                    if (window1EnabledOnScanline_)
                    {
                        ConfigureNonObjWindow(x1, x2, window1);
                    }
                }

                if (dispcnt_.window0Display)
                {
                    #pragma GCC diagnostic push
                    #pragma GCC diagnostic ignored "-Wnarrowing"
                    WindowSettings window0 = {
                        {winin.win0Bg0Enabled, winin.win0Bg1Enabled, winin.win0Bg2Enabled, winin.win0Bg3Enabled},
                        winin.win0ObjEnabled,
                        winin.win0SpecialEffect
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

            if (dispcnt_.screenDisplayObj)
            {
                frameBuffer_.ClearSpritePixels();
                EvaluateOAM();
                frameBuffer_.PushSpritePixels();
            }

            switch (dispcnt_.bgMode)
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

        BLDCNT const& bldcnt = *reinterpret_cast<BLDCNT const*>(&lcdRegisters_[0x50]);
        BLDALPHA const& bldalpha = *reinterpret_cast<BLDALPHA const*>(&lcdRegisters_[0x52]);
        BLDY const& bldy = *reinterpret_cast<BLDY const*>(&lcdRegisters_[0x54]);

        frameBuffer_.RenderScanline(backdrop, forceBlank, bldcnt, bldalpha, bldy);
        IncrementAffineBackgroundReferencePoints();
    }
}

void PPU::VBlank(int extraCycles)
{
    // VBlank register updates
    ++scanline_;
    vcount_.currentScanline = scanline_;
    dispstat_.hBlank = 0;

    if (scanline_ == 160)
    {
        // First time entering VBlank
        dispstat_.vBlank = 1;
        ++frameCounter_;
        frameBuffer_.ResetFrameIndex();

        if (dispstat_.vBlankIrqEnable)
        {
            SystemController.RequestInterrupt(InterruptType::LCD_VBLANK);
        }

        bg2RefX_ = SignExtend32(*reinterpret_cast<uint32_t*>(&lcdRegisters_[0x28]), 27);
        bg2RefY_ = SignExtend32(*reinterpret_cast<uint32_t*>(&lcdRegisters_[0x2C]), 27);
        bg3RefX_ = SignExtend32(*reinterpret_cast<uint32_t*>(&lcdRegisters_[0x38]), 27);
        bg3RefY_ = SignExtend32(*reinterpret_cast<uint32_t*>(&lcdRegisters_[0x3C]), 27);
    }
    else if (scanline_ == 227)
    {
        dispstat_.vBlank = 0;
    }

    if (scanline_ == dispstat_.vCountSetting)
    {
        dispstat_.vCounter = 1;

        if (dispstat_.vCounterIrqEnable)
        {
            SystemController.RequestInterrupt(InterruptType::LCD_VCOUNTER_MATCH);
        }
    }
    else
    {
        dispstat_.vCounter = 0;
    }

    SetNonObjWindowEnabled();

    // Event Scheduling
    Scheduler.ScheduleEvent(EventType::HBlank, (960 - extraCycles) + 46);
}

void PPU::VDraw(int extraCycles)
{
    // VDraw register updates
    ++scanline_;

    if (scanline_ == 228)
    {
        scanline_ = 0;
    }

    vcount_.currentScanline = scanline_;
    dispstat_.hBlank = 0;

    if (scanline_ == dispstat_.vCountSetting)
    {
        dispstat_.vCounter = 1;

        if (dispstat_.vCounterIrqEnable)
        {
            SystemController.RequestInterrupt(InterruptType::LCD_VCOUNTER_MATCH);
        }
    }
    else
    {
        dispstat_.vCounter = 0;
    }

    SetNonObjWindowEnabled();

    // Event Scheduling
    int cyclesUntilHBlank = (960 - extraCycles) + 46;
    Scheduler.ScheduleEvent(EventType::HBlank, cyclesUntilHBlank);
}

void PPU::WriteDispstatVcount(uint32_t addr, uint32_t value, AccessSize alignment)
{
    if (addr < 0x0400'0006)  // Write to DISPSTAT and possibly VCOUNT. Ignore writes to any portion of VCOUNT.
    {
        if (alignment != AccessSize::BYTE)  // Completely write DISPSTAT.
        {
            uint16_t writableMask = 0xFFB8;
            dispstat_.value &= ~writableMask;
            value = (value & MAX_U16) & writableMask;
            dispstat_.value |= value;
        }
        else if (addr == 0x0400'0004)  // BYTE write to lower half of DISPSTAT.
        {
            uint16_t writableMask = 0x00B8;
            dispstat_.value &= ~writableMask;
            value = (value & MAX_U8) & writableMask;
            dispstat_.value |= value;
        }
        else  // BYTE write to upper half of DISPSTAT.
        {
            uint16_t writableMask = 0xFF00;
            dispstat_.value &= ~writableMask;
            value = ((value & MAX_U8) << 8) & writableMask;
            dispstat_.value |= value;
        }
    }
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
        if (dispcnt_.value & (0x100 << i))
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
        if (dispcnt_.value & (0x100 << i))
        {
            BGCNT const& bgControl = *reinterpret_cast<BGCNT*>(&lcdRegisters_[0x08 + (2 * i)]);
            uint16_t xOffset = *reinterpret_cast<uint16_t*>(&lcdRegisters_[0x10 + (4 * i)]) & 0x01FF;
            uint16_t yOffset = *reinterpret_cast<uint16_t*>(&lcdRegisters_[0x12 + (4 * i)]) & 0x01FF;
            RenderRegularTiledBackgroundScanline(i, bgControl, xOffset, yOffset);
        }
    }

    if (dispcnt_.screenDisplayBg2)
    {
        BGCNT const& bgControl = *reinterpret_cast<BGCNT*>(&lcdRegisters_[0x0C]);
        int16_t pa = *reinterpret_cast<int16_t*>(&lcdRegisters_[0x20]);
        int16_t pc = *reinterpret_cast<int16_t*>(&lcdRegisters_[0x24]);
        RenderAffineTiledBackgroundScanline(2, bgControl, bg2RefX_, bg2RefY_, pa, pc);
    }
}

void PPU::RenderMode2Scanline()
{
    if (dispcnt_.screenDisplayBg2)
    {
        BGCNT const& bgControl = *reinterpret_cast<BGCNT*>(&lcdRegisters_[0x0C]);
        int16_t pa = *reinterpret_cast<int16_t*>(&lcdRegisters_[0x20]);
        int16_t pc = *reinterpret_cast<int16_t*>(&lcdRegisters_[0x24]);
        RenderAffineTiledBackgroundScanline(2, bgControl, bg2RefX_, bg2RefY_, pa, pc);
    }

    if (dispcnt_.screenDisplayBg3)
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

    if (dispcnt_.screenDisplayBg2)
    {
        for (int dot = 0; dot < 240; ++dot)
        {
            if (frameBuffer_.GetWindowSettings(dot).bgEnabled_[2])
            {
                frameBuffer_.PushPixel({PixelSrc::BG2, *vramPtr, bgControl.bgPriority, false}, dot);
            }

            ++vramPtr;
        }
    }
}

void PPU::RenderMode4Scanline()
{
    BGCNT const& bgControl = *reinterpret_cast<BGCNT*>(&lcdRegisters_[0x0C]);
    size_t vramIndex = scanline_ * 240;
    uint16_t const* palettePtr = reinterpret_cast<uint16_t const*>(PRAM_.data());

    if (dispcnt_.displayFrameSelect)
    {
        vramIndex += 0xA000;
    }

    if (dispcnt_.screenDisplayBg2)
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
                frameBuffer_.PushPixel({PixelSrc::BG2, bgr555, bgControl.bgPriority, transparent}, dot);
            }
        }
    }
}

void PPU::RenderRegularTiledBackgroundScanline(int bgIndex, BGCNT const& control, int xOffset, int yOffset)
{
    int const width = (control.screenSize & 0b01) ? 512 : 256;
    int const height = (control.screenSize & 0b10) ? 512 : 256;

    int const x = xOffset % width;
    int const y = (scanline_ + yOffset) % height;

    if (control.colorMode)
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
        reinterpret_cast<ScreenBlock const*>(&VRAM_[control.screenBaseBlock * SCREENBLOCK_SIZE]);

    if (y > 255)
    {
        ++screenBlockPtr;

        if (width == 512)
        {
            ++screenBlockPtr;
        }
    }

    int const mapY = (y / 8) % 32;
    TileData4bpp const* baseTilePtr = reinterpret_cast<TileData4bpp const*>(&VRAM_[control.charBaseBlock * CHARBLOCK_SIZE]);
    ScreenBlockEntry const* screenBlockEntryPtr = nullptr;
    uint16_t const* palettePtr = reinterpret_cast<uint16_t const*>(&PRAM_[0]);

    // Control data
    PixelSrc const src = static_cast<PixelSrc>(bgIndex + 1);
    int const priority = control.bgPriority;

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
        reinterpret_cast<ScreenBlock const*>(&VRAM_[control.screenBaseBlock * SCREENBLOCK_SIZE]);

    if (y > 255)
    {
        ++screenBlockPtr;

        if (width == 512)
        {
            ++screenBlockPtr;
        }
    }

    int const mapY = (y / 8) % 32;
    size_t const charBlockAddr = control.charBaseBlock * CHARBLOCK_SIZE;
    TileData8bpp const* baseTilePtr = reinterpret_cast<TileData8bpp const*>(&VRAM_[charBlockAddr]);
    ScreenBlockEntry const* screenBlockEntryPtr = nullptr;
    uint16_t const* palettePtr = reinterpret_cast<uint16_t const*>(&PRAM_[0]);

    // Control data
    PixelSrc const src = static_cast<PixelSrc>(bgIndex + 1);
    int const priority = control.bgPriority;

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

    switch (control.screenSize)
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
    bool const wrapOnOverflow = control.overflowMode;
    int const priority = control.bgPriority;
    PixelSrc const src = static_cast<PixelSrc>(bgIndex + 1);

    // VRAM pointers
    uint8_t const* screenBlockPtr = &VRAM_[control.screenBaseBlock * SCREENBLOCK_SIZE];
    TileData8bpp const* tilePtr = reinterpret_cast<TileData8bpp const*>(&VRAM_[control.charBaseBlock * CHARBLOCK_SIZE]);
    uint16_t const* palettePtr = reinterpret_cast<uint16_t const*>(PRAM_.data());

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
    OamEntry const* oam = reinterpret_cast<OamEntry const*>(OAM_.data());
    bool const evaluateWindowSprites = (windowSettingsPtr != nullptr);

    for (int i = 0; i < 128; ++i)
    {
        OamEntry const& oamEntry = oam[i];

        // Skip window sprites when evaluating visible sprites, and vice versa
        if ((evaluateWindowSprites && (oamEntry.attribute0_.gfxMode_ != 2)) ||
            (!evaluateWindowSprites && (oamEntry.attribute0_.gfxMode_ == 2)))
        {
            continue;
        }

        // Skip disabled sprites and illegal sprites
        if ((oamEntry.attribute0_.objMode_ == 2) || (oamEntry.attribute0_.gfxMode_ == 3))
        {
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

        int topEdge = y;
        int bottomEdge = y + height - 1;

        if (oamEntry.attribute0_.objMode_ == 3)
        {
            y += (height / 2);
            x += (width / 2);
            topEdge = y - (height / 2);
            bottomEdge = topEdge + (2 * height) - 1;
        }

        // Check if this sprite exists on the current scanline
        if ((topEdge > scanline_) || (scanline_ > bottomEdge))
        {
            continue;
        }

        if (dispcnt_.objCharacterVramMapping)
        {
            // One dimensional mapping
            if (oamEntry.attribute0_.colorMode_)
            {
                // 8bpp
                if (oamEntry.attribute0_.objMode_ == 0)
                {
                    Render1d8bppRegularSprite(x, y, width, height, oamEntry, windowSettingsPtr);
                }
                else
                {
                    Render1d8bppAffineSprite(x, y, width, height, oamEntry, windowSettingsPtr);
                }
            }
            else
            {
                // 4bpp
                if (oamEntry.attribute0_.objMode_ == 0)
                {
                    Render1d4bppRegularSprite(x, y, width, height, oamEntry, windowSettingsPtr);
                }
                else
                {
                    Render1d4bppAffineSprite(x, y, width, height, oamEntry, windowSettingsPtr);
                }
            }
        }
        else
        {
            // Two dimensional mapping
            if (oamEntry.attribute0_.colorMode_)
            {
                // 8bpp
                if (oamEntry.attribute0_.objMode_ == 0)
                {
                    Render2d8bppRegularSprite(x, y, width, height, oamEntry, windowSettingsPtr);
                }
                else
                {
                    Render2d8bppAffineSprite(x, y, width, height, oamEntry, windowSettingsPtr);
                }
            }
            else
            {
                // 4bpp
                if (oamEntry.attribute0_.objMode_ == 0)
                {
                    Render2d4bppRegularSprite(x, y, width, height, oamEntry, windowSettingsPtr);
                }
                else
                {
                    Render2d4bppAffineSprite(x, y, width, height, oamEntry, windowSettingsPtr);
                }
            }
        }
    }
}

void PPU::Render1d4bppRegularSprite(int x, int y, int width, int height, OamEntry const& oamEntry, WindowSettings* windowSettingsPtr)
{
    int const widthInTiles = width / 8;
    int const heightInTiles = height / 8;

    TileData4bpp const* const tileMapPtr = reinterpret_cast<TileData4bpp const*>(&VRAM_[OBJ_CHARBLOCK_ADDR]);
    uint16_t const* const palettePtr = reinterpret_cast<uint16_t const*>(&PRAM_[OBJ_PALETTE_ADDR]);

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
    bool const semiTransparent = (oamEntry.attribute0_.gfxMode_ == 1);

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
            AddSpritePixelToLineBuffer(dot, bgr555, priority, transparent, semiTransparent, windowSettingsPtr);

            --pixelsToDraw;
            ++dot;
            leftHalf = !leftHalf;
            tileX += (horizontalFlip ? -1 : 1);
        }
    }
}

void PPU::Render1d8bppRegularSprite(int x, int y, int width, int height, OamEntry const& oamEntry, WindowSettings* windowSettingsPtr)
{
    TileData8bpp const* tileDataPtr = nullptr;
    uint16_t const* const palettePtr = reinterpret_cast<uint16_t const*>(&PRAM_[OBJ_PALETTE_ADDR]);

    int const widthInTiles = width / 8;
    int const heightInTiles = height / 8;

    bool const verticalFlip = oamEntry.attribute1_.noRotationOrScaling_.verticalFlip_;
    bool const horizontalFlip = oamEntry.attribute1_.noRotationOrScaling_.horizontalFlip_;

    size_t const verticalOffset = verticalFlip ? (heightInTiles - ((scanline_ - y) / 8) - 1) * widthInTiles * sizeof(TileData8bpp) :
                                                 ((scanline_ - y) / 8) * widthInTiles * sizeof(TileData8bpp);

    size_t const baseVramIndex = (oamEntry.attribute2_.tile_ * sizeof(TileData4bpp)) + verticalOffset;

    size_t const tileY = verticalFlip ?
        ((scanline_ - y) % 8) ^ 7 :
        (scanline_ - y) % 8;

    int const leftEdge = std::max(0, x);
    int const rightEdge = std::min(240, x + width);
    int const priority = oamEntry.attribute2_.priority_;
    bool const semiTransparent = (oamEntry.attribute0_.gfxMode_ == 1);

    for (int dot = leftEdge; dot < rightEdge; ++dot)
    {
        if (((dot - x) % 8) == 0)
        {
            tileDataPtr = nullptr;
        }

        if (tileDataPtr == nullptr)
        {
            size_t horizontalOffset = horizontalFlip ? (widthInTiles - ((dot - x) / 8) - 1) * sizeof(TileData8bpp) :
                                                       ((dot - x) / 8) * sizeof(TileData8bpp);

            size_t vramIndex = OBJ_CHARBLOCK_ADDR + ((baseVramIndex + horizontalOffset) % 0x8000);

            if ((vramIndex + sizeof(TileData8bpp)) >= VRAM_.size())
            {
                continue;
            }

            tileDataPtr = reinterpret_cast<TileData8bpp const*>(&VRAM_[vramIndex]);
        }

        size_t tileX = horizontalFlip ?
            ((dot - x) % 8) ^ 7 :
            (dot - x) % 8;

        size_t paletteIndex = tileDataPtr->paletteIndex_[tileY][tileX];
        bool transparent = (paletteIndex == 0);
        uint16_t bgr555 = palettePtr[paletteIndex];
        AddSpritePixelToLineBuffer(dot, bgr555, priority, transparent, semiTransparent, windowSettingsPtr);
    }
}

void PPU::Render2d4bppRegularSprite(int x, int y, int width, int height, OamEntry const& oamEntry, WindowSettings* windowSettingsPtr)
{
    TwoDim4bppMap const* const tileMapPtr = reinterpret_cast<TwoDim4bppMap const*>(&VRAM_[OBJ_CHARBLOCK_ADDR]);
    uint16_t const* const palettePtr = reinterpret_cast<uint16_t const*>(&PRAM_[OBJ_PALETTE_ADDR]);
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
    bool const semiTransparent = (oamEntry.attribute0_.gfxMode_ == 1);

    while (dot <= rightEdge)
    {
        if (tileDataPtr == nullptr)
        {
            mapX = horizontalFlip ?
                (baseMapX + (widthInTiles - ((dot - x) / 8) - 1)) % 32 :
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
            AddSpritePixelToLineBuffer(dot, bgr555, priority, transparent, semiTransparent, windowSettingsPtr);

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

void PPU::Render2d8bppRegularSprite(int x, int y, int width, int height, OamEntry const& oamEntry, WindowSettings* windowSettingsPtr)
{
    TwoDim8bppMap const* const tileMapPtr = reinterpret_cast<TwoDim8bppMap const*>(&VRAM_[OBJ_CHARBLOCK_ADDR]);
    uint16_t const* const palettePtr = reinterpret_cast<uint16_t const*>(&PRAM_[OBJ_PALETTE_ADDR]);
    TileData8bpp const* tileDataPtr = nullptr;

    int const leftEdge = std::max(0, x);
    int const rightEdge = std::min(LCD_WIDTH, x + width);

    int const widthInTiles = width / 8;
    int const heightInTiles = height / 8;

    int const priority = oamEntry.attribute2_.priority_;
    bool const semiTransparent = (oamEntry.attribute0_.gfxMode_ == 1);
    bool const verticalFlip = oamEntry.attribute1_.noRotationOrScaling_.verticalFlip_;
    bool const horizontalFlip = oamEntry.attribute1_.noRotationOrScaling_.horizontalFlip_;

    size_t const tileIndex = oamEntry.attribute2_.tile_ / 2;
    size_t const baseMapX = tileIndex % 16;
    size_t const baseMapY = (tileIndex / 16) % 32;

    size_t const mapY = verticalFlip ?
        (baseMapY + (heightInTiles - ((scanline_ - y) / 8) - 1)) % 32 :
        (baseMapY + ((scanline_ - y) / 8)) % 32;

    size_t const tileY = verticalFlip ?
        ((scanline_ - y) % 8) ^ 7 :
        (scanline_ - y) % 8;

    for (int dot = leftEdge; dot < rightEdge; ++dot)
    {
        if (((dot - x) % 8) == 0)
        {
            tileDataPtr = nullptr;
        }

        if (tileDataPtr == nullptr)
        {
            size_t mapX = horizontalFlip ?
                (baseMapX + (widthInTiles - ((dot - x) / 8) - 1)) % 16 :
                (baseMapX + ((dot - x) / 8)) % 16;

            tileDataPtr = &tileMapPtr->tileData_[mapY][mapX];
        }

        size_t tileX = horizontalFlip ?
            ((dot - x) % 8) ^ 7 :
            (dot - x) % 8;

        size_t paletteIndex = tileDataPtr->paletteIndex_[tileY][tileX];
        bool transparent = (paletteIndex == 0);
        uint16_t bgr555 = palettePtr[paletteIndex];
        AddSpritePixelToLineBuffer(dot, bgr555, priority, transparent, semiTransparent, windowSettingsPtr);
    }
}

void PPU::Render1d4bppAffineSprite(int x, int y, int width, int height, OamEntry const& oamEntry, WindowSettings* windowSettingsPtr)
{
    TileData4bpp const* tileMapPtr = reinterpret_cast<TileData4bpp const*>(&VRAM_[OBJ_CHARBLOCK_ADDR]);
    uint16_t const* palettePtr = reinterpret_cast<uint16_t const*>(&PRAM_[OBJ_PALETTE_ADDR]);
    AffineObjMatrix const* affineMatrix =
        &(reinterpret_cast<AffineObjMatrix const*>(&OAM_[0])[oamEntry.attribute1_.rotationOrScaling_.parameterSelection_]);

    int16_t const pa = affineMatrix->pa_;
    int16_t const pb = affineMatrix->pb_;
    int16_t const pc = affineMatrix->pc_;
    int16_t const pd = affineMatrix->pd_;

    int leftEdge = x;
    int rightEdge = x + width - 1;
    int topEdge = y;
    int const halfWidth = width / 2;
    int const halfHeight = height / 2;
    bool doubleSize = (oamEntry.attribute0_.objMode_ == 3);

    if (doubleSize)
    {
        leftEdge -= halfWidth;
        rightEdge += halfWidth;
        topEdge -= halfHeight;
    }

    // Rotation center
    int16_t const x0 = doubleSize ? width : halfWidth;
    int16_t const y0 = doubleSize ? height : halfHeight;

    // Screen position
    int16_t const x1 = 0;
    int16_t const y1 = scanline_ - topEdge;

    int32_t affineX = (pa * (x1 - x0)) + (pb * (y1 - y0)) + (halfWidth << 8);
    int32_t affineY = (pc * (x1 - x0)) + (pd * (y1 - y0)) + (halfHeight << 8);

    size_t const palette = oamEntry.attribute2_.palette_ << 4;
    int const priority = oamEntry.attribute2_.priority_;
    bool const semiTransparent = (oamEntry.attribute0_.gfxMode_ == 1);
    size_t const widthInTiles = width / 8;
    size_t const baseTileIndex = oamEntry.attribute2_.tile_;

    for (int dot = leftEdge; (dot <= rightEdge) && (dot < LCD_WIDTH); ++dot)
    {
        int32_t textureX = (affineX >> 8);
        int32_t textureY = (affineY >> 8);
        affineX += pa;
        affineY += pc;

        if ((dot < 0) || (textureX < 0) || (textureX >= width) || (textureY < 0) || (textureY >= height))
        {
            continue;
        }

        size_t tileOffset = ((textureX / 8) % widthInTiles) + ((textureY / 8) * widthInTiles);
        TileData4bpp const* tileDataPtr = &tileMapPtr[(baseTileIndex + tileOffset) % 1024];

        size_t tileX = textureX % 8;
        size_t tileY = textureY % 8;
        bool left = (tileX % 2) == 0;
        auto tileNibbles = tileDataPtr->paletteIndex_[tileY][tileX / 2];
        size_t paletteIndex = palette | (left ? tileNibbles.leftNibble_ : tileNibbles.rightNibble_);
        bool transparent = (paletteIndex & 0x0F) == 0;
        uint16_t bgr555 = palettePtr[paletteIndex];
        AddSpritePixelToLineBuffer(dot, bgr555, priority, transparent, semiTransparent, windowSettingsPtr);
    }
}

void PPU::Render2d4bppAffineSprite(int x, int y, int width, int height, OamEntry const& oamEntry, WindowSettings* windowSettingsPtr)
{
    TwoDim4bppMap const* tileMapPtr = reinterpret_cast<TwoDim4bppMap const*>(&VRAM_[OBJ_CHARBLOCK_ADDR]);
    uint16_t const* palettePtr = reinterpret_cast<uint16_t const*>(&PRAM_[OBJ_PALETTE_ADDR]);
    AffineObjMatrix const* affineMatrix =
        &(reinterpret_cast<AffineObjMatrix const*>(&OAM_[0])[oamEntry.attribute1_.rotationOrScaling_.parameterSelection_]);

    int16_t const pa = affineMatrix->pa_;
    int16_t const pb = affineMatrix->pb_;
    int16_t const pc = affineMatrix->pc_;
    int16_t const pd = affineMatrix->pd_;

    int leftEdge = x;
    int rightEdge = x + width;
    int topEdge = y;
    int const halfWidth = width / 2;
    int const halfHeight = height / 2;
    bool doubleSize = (oamEntry.attribute0_.objMode_ == 3);

    if (doubleSize)
    {
        leftEdge -= halfWidth;
        rightEdge += halfWidth;
        topEdge -= halfHeight;
    }

    // Rotation center
    int16_t const x0 = doubleSize ? width : halfWidth;
    int16_t const y0 = doubleSize ? height : halfHeight;

    // Screen position
    int16_t const x1 = 0;
    int16_t const y1 = scanline_ - topEdge;

    int32_t affineX = (pa * (x1 - x0)) + (pb * (y1 - y0)) + (halfWidth << 8);
    int32_t affineY = (pc * (x1 - x0)) + (pd * (y1 - y0)) + (halfHeight << 8);

    size_t const palette = oamEntry.attribute2_.palette_ << 4;
    int const priority = oamEntry.attribute2_.priority_;
    bool const semiTransparent = (oamEntry.attribute0_.gfxMode_ == 1);
    size_t const baseMapX = oamEntry.attribute2_.tile_ % 32;
    size_t const baseMapY = oamEntry.attribute2_.tile_ / 32;

    for (int dot = x; (dot < rightEdge) && (dot < LCD_WIDTH); ++dot)
    {
        int32_t textureX = (affineX >> 8);
        int32_t textureY = (affineY >> 8);
        affineX += pa;
        affineY += pc;

        if ((dot < 0) || (textureX < 0) || (textureX >= width) || (textureY < 0) || (textureY >= height))
        {
            continue;
        }

        size_t mapX = (baseMapX + (textureX / 8)) % 32;
        size_t mapY = (baseMapY + (textureY / 8)) % 32;
        size_t tileX = textureX % 8;
        size_t tileY = textureY % 8;
        bool left = (tileX % 2) == 0;
        TileData4bpp const* tileDataPtr = &tileMapPtr->tileData_[mapY][mapX];
        auto tileNibbles = tileDataPtr->paletteIndex_[tileY][tileX / 2];
        size_t paletteIndex = palette | (left ? tileNibbles.leftNibble_ : tileNibbles.rightNibble_);
        bool transparent = (paletteIndex & 0x0F) == 0;

        if (transparent)
        {
            paletteIndex = 0;
        }

        uint16_t bgr555 = palettePtr[paletteIndex];
        AddSpritePixelToLineBuffer(dot, bgr555, priority, transparent, semiTransparent, windowSettingsPtr);
    }
}

void PPU::Render1d8bppAffineSprite(int x, int y, int width, int height, OamEntry const& oamEntry, WindowSettings* windowSettingsPtr)
{
    uint16_t const* palettePtr = reinterpret_cast<uint16_t const*>(&PRAM_[OBJ_PALETTE_ADDR]);
    AffineObjMatrix const* affineMatrix =
        &(reinterpret_cast<AffineObjMatrix const*>(&OAM_[0])[oamEntry.attribute1_.rotationOrScaling_.parameterSelection_]);

    int16_t const pa = affineMatrix->pa_;
    int16_t const pb = affineMatrix->pb_;
    int16_t const pc = affineMatrix->pc_;
    int16_t const pd = affineMatrix->pd_;

    int leftEdge = x;
    int rightEdge = x + width;
    int topEdge = y;
    int const halfWidth = width / 2;
    int const halfHeight = height / 2;
    bool doubleSize = (oamEntry.attribute0_.objMode_ == 3);

    if (doubleSize)
    {
        leftEdge -= halfWidth;
        rightEdge += halfWidth;
        topEdge -= halfHeight;
    }

    // Rotation center
    int16_t const x0 = doubleSize ? width : halfWidth;
    int16_t const y0 = doubleSize ? height : halfHeight;

    // Screen position
    int16_t const x1 = 0;
    int16_t const y1 = scanline_ - topEdge;

    int32_t affineX = (pa * (x1 - x0)) + (pb * (y1 - y0)) + (halfWidth << 8);
    int32_t affineY = (pc * (x1 - x0)) + (pd * (y1 - y0)) + (halfHeight << 8);

    int const priority = oamEntry.attribute2_.priority_;
    bool const semiTransparent = (oamEntry.attribute0_.gfxMode_ == 1);
    size_t const widthInTiles = width / 8;
    size_t const baseVramIndex = oamEntry.attribute2_.tile_ * sizeof(TileData4bpp);

    for (int dot = leftEdge; (dot < rightEdge) && (dot < LCD_WIDTH); ++dot)
    {
        int32_t textureX = (affineX >> 8);
        int32_t textureY = (affineY >> 8);
        affineX += pa;
        affineY += pc;

        if ((dot < 0) || (textureX < 0) || (textureX >= width) || (textureY < 0) || (textureY >= height))
        {
            continue;
        }

        size_t offset = (((textureX / 8) % widthInTiles) + ((textureY / 8) * widthInTiles)) * sizeof(TileData8bpp);
        size_t vramIndex = OBJ_CHARBLOCK_ADDR + ((baseVramIndex + offset) % (2 * CHARBLOCK_SIZE));

        if ((vramIndex + sizeof(TileData8bpp)) >= VRAM_.size())
        {
            continue;
        }

        TileData8bpp const* tileDataPtr = reinterpret_cast<TileData8bpp const*>(&VRAM_[vramIndex]);

        size_t tileX = textureX % 8;
        size_t tileY = textureY % 8;
        size_t paletteIndex = tileDataPtr->paletteIndex_[tileY][tileX];
        bool transparent = (paletteIndex == 0);
        uint16_t bgr555 = palettePtr[paletteIndex];
        AddSpritePixelToLineBuffer(dot, bgr555, priority, transparent, semiTransparent, windowSettingsPtr);
    }
}

void PPU::Render2d8bppAffineSprite(int x, int y, int width, int height, OamEntry const& oamEntry, WindowSettings* windowSettingsPtr)
{
    TwoDim8bppMap const* tileMapPtr = reinterpret_cast<TwoDim8bppMap const*>(&VRAM_[OBJ_CHARBLOCK_ADDR]);
    uint16_t const* palettePtr = reinterpret_cast<uint16_t const*>(&PRAM_[OBJ_PALETTE_ADDR]);
    AffineObjMatrix const* affineMatrix =
        &(reinterpret_cast<AffineObjMatrix const*>(&OAM_[0])[oamEntry.attribute1_.rotationOrScaling_.parameterSelection_]);

    int16_t const pa = affineMatrix->pa_;
    int16_t const pb = affineMatrix->pb_;
    int16_t const pc = affineMatrix->pc_;
    int16_t const pd = affineMatrix->pd_;

    int leftEdge = x;
    int rightEdge = x + width;
    int topEdge = y;
    int const halfWidth = width / 2;
    int const halfHeight = height / 2;
    bool doubleSize = (oamEntry.attribute0_.objMode_ == 3);

    if (doubleSize)
    {
        leftEdge -= halfWidth;
        rightEdge += halfWidth;
        topEdge -= halfHeight;
    }

    // Rotation center
    int16_t const x0 = doubleSize ? width : halfWidth;
    int16_t const y0 = doubleSize ? height : halfHeight;

    // Screen position
    int16_t const x1 = 0;
    int16_t const y1 = scanline_ - topEdge;

    int32_t affineX = (pa * (x1 - x0)) + (pb * (y1 - y0)) + (halfWidth << 8);
    int32_t affineY = (pc * (x1 - x0)) + (pd * (y1 - y0)) + (halfHeight << 8);

    int const priority = oamEntry.attribute2_.priority_;
    bool const semiTransparent = (oamEntry.attribute0_.gfxMode_ == 1);

    size_t const tileIndex = oamEntry.attribute2_.tile_ / 2;
    size_t const baseMapX = tileIndex % 16;
    size_t const baseMapY = (tileIndex / 16) % 32;

    for (int dot = x; (dot < rightEdge) && (dot < LCD_WIDTH); ++dot)
    {
        int32_t textureX = (affineX >> 8);
        int32_t textureY = (affineY >> 8);
        affineX += pa;
        affineY += pc;

        if ((dot < 0) || (textureX < 0) || (textureX >= width) || (textureY < 0) || (textureY >= height))
        {
            continue;
        }

        size_t mapX = (baseMapX + (textureX / 8)) % 16;
        size_t mapY = (baseMapY + (textureY / 8)) % 32;
        size_t tileX = textureX % 8;
        size_t tileY = textureY % 8;
        TileData8bpp const* tileDataPtr = &tileMapPtr->tileData_[mapY][mapX];
        size_t paletteIndex = tileDataPtr->paletteIndex_[tileY][tileX];
        bool transparent = (paletteIndex == 0);
        uint16_t bgr555 = palettePtr[paletteIndex];
        AddSpritePixelToLineBuffer(dot, bgr555, priority, transparent, semiTransparent, windowSettingsPtr);
    }
}

void PPU::AddSpritePixelToLineBuffer(int dot, uint16_t bgr555, int priority, bool transparent, bool semiTransparent, WindowSettings* windowSettingsPtr)
{
    if (windowSettingsPtr == nullptr)
    {
        // Visible Sprite
        Pixel& currentPixel = frameBuffer_.GetSpritePixel(dot);

        if (frameBuffer_.GetWindowSettings(dot).objEnabled_ && !transparent &&
            (!currentPixel.initialized_ || (priority < currentPixel.priority_) || currentPixel.transparent_))
        {
            currentPixel = Pixel(PixelSrc::OBJ, bgr555, priority, transparent, semiTransparent);
        }
    }
    else if (!transparent)
    {
        // Opaque OBJ window sprite pixel
        frameBuffer_.GetWindowSettings(dot) = *windowSettingsPtr;
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
