#include <Graphics/PPU.hpp>
#include <System/InterruptManager.hpp>
#include <System/MemoryMap.hpp>
#include <System/Scheduler.hpp>
#include <System/Utilities.hpp>
#include <cstdint>
#include <format>
#include <functional>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace Graphics
{
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

    // Draw scanline
    uint8_t bgMode = lcdControl_.flags_.bgMode;

    switch (bgMode)
    {
        case 3:
            RenderMode3Scanline();
            break;
        case 4:
            RenderMode4Scanline();
            break;
        default:
            throw std::runtime_error(std::format("Unimplemented BG mode: {}", bgMode));
    }
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

void PPU::RenderMode3Scanline()
{
    size_t vramIndex = scanline_ * 480;
    uint16_t const* vramPtr = reinterpret_cast<uint16_t const*>(&VRAM_.at(vramIndex));

    for (int i = 0; i < 240; ++i)
    {
        frameBuffer_.WritePixel(*vramPtr);
        ++vramPtr;
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

    for (int i = 0; i < 240; ++i)
    {
        uint8_t paletteIndex = VRAM_.at(vramIndex++);
        frameBuffer_.WritePixel(palettePtr[paletteIndex]);
    }
}
}
