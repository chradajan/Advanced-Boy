#include <Graphics/PPU.hpp>
#include <MemoryMap.hpp>
#include <System/InterruptManager.hpp>
#include <System/Scheduler.hpp>
#include <cstdint>
#include <format>
#include <functional>
#include <stdexcept>
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

std::pair<uint32_t, int> PPU::ReadLcdReg(uint32_t addr, uint8_t accessSize)
{
    // TODO Ensure memory safety by checking that accessSize doesn't allow reads beyond registers array.
    uint8_t* bytePtr = &lcdRegisters_.at(addr - LCD_IO_ADDR_MIN);
    uint32_t regValue;

    switch (accessSize)
    {
        case 1:
            regValue = *bytePtr;
            break;
        case 2:
            regValue = *reinterpret_cast<uint16_t*>(bytePtr);
            break;
        case 4:
            regValue = *reinterpret_cast<uint32_t*>(bytePtr);
            break;
        default:
            throw std::runtime_error("Illegal PPU register read access size");
    }

    // TODO Implement proper access times
    return {regValue, 1};
}

int PPU::WriteLcdReg(uint32_t addr, uint32_t val, uint8_t accessSize)
{
    // TODO Ensure memory safety by checking that accessSize doesn't allow writes beyond registers array.
    uint8_t* bytePtr = &lcdRegisters_.at(addr - LCD_IO_ADDR_MIN);

    switch (accessSize)
    {
        case 1:
            *bytePtr = val;
            break;
        case 2:
            *reinterpret_cast<uint16_t*>(bytePtr) = val;
            break;
        case 4:
            *reinterpret_cast<uint32_t*>(bytePtr) = val;
            break;
        default:
            throw std::runtime_error("Illegal PPU register write access size");
    }

    // TODO Implement proper access times
    return 1;
}

void PPU::VDraw(int extraCycles)
{
    // VDraw register updates
    ++scanline_;

    if (scanline_ == 228)
    {
        scanline_ = 0;
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

    // Event Scheduling
    int cyclesUntilHBlank = 960 - extraCycles;
    Scheduler.ScheduleEvent(EventType::HBlank, cyclesUntilHBlank);

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

void PPU::HBlank(int extraCycles)
{
    // HBlank register updates
    lcdStatus_.flags_.hBlank = 1;

    if (lcdStatus_.flags_.hBlankIrqEnable)
    {
        InterruptMgr.RequestInterrupt(InterruptType::LCD_HBLANK);
    }

    // Event scheduling
    int cyclesUntilNextEvent = 272 - extraCycles;
    EventType nextEvent = (scanline_ == 159) ? EventType::VBlank : EventType::VDraw;
    Scheduler.ScheduleEvent(nextEvent, cyclesUntilNextEvent);
}

void PPU::VBlank(int extraCycles)
{
    // VBlank register updates
    ++scanline_;
    lcdStatus_.flags_.vBlank = 1;
    verticalCounter_.flags_.currentScanline = scanline_;

    if (lcdStatus_.flags_.vBlankIrqEnable)
    {
        InterruptMgr.RequestInterrupt(InterruptType::LCD_VBLANK);
    }

    if (scanline_ == lcdStatus_.flags_.vCountSetting)
    {
        lcdStatus_.flags_.vCounter = 1;

        if (lcdStatus_.flags_.vCounterIrqEnable)
        {
            InterruptMgr.RequestInterrupt(InterruptType::LCD_VCOUNTER_MATCH);
        }
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
    uint16_t const* const palettePtr = reinterpret_cast<uint16_t const*>(paletteRAM_.data());

    if (lcdControl_.flags_.displayFrameSelect)
    {
        vramIndex += 0x9600;
    }

    for (int i = 0; i < 240; ++i)
    {
        uint8_t paletteIndex = VRAM_.at(vramIndex++);
        frameBuffer_.WritePixel(palettePtr[paletteIndex]);
    }
}
}
