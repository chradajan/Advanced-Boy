#include <Graphics/PPU.hpp>
#include <MemoryMap.hpp>
#include <System/InterruptManager.hpp>
#include <System/Scheduler.hpp>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <utility>

namespace Graphics
{
PPU::PPU(std::array<uint8_t,   1 * KiB> const& paletteRAM,
         std::array<uint8_t,  96 * KiB> const& VRAM,
         std::array<uint8_t,   1 * KiB> const& OAM) :
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
}
