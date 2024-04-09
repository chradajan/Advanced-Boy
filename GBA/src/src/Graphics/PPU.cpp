#include <Graphics/PPU.hpp>
#include <Memory/Memory.hpp>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace Graphics
{
PPU::PPU() :
    lcdRegisters_(),
    lcdControl_(*reinterpret_cast<DISPCNT*>(&lcdRegisters_[0])),
    lcdStatus_(*reinterpret_cast<DISPSTAT*>(&lcdRegisters_[4])),
    verticalCounter_(*reinterpret_cast<VCOUNT*>(&lcdRegisters_[6]))
{
    lcdRegisters_.fill(0);
}

std::pair<uint32_t, int> PPU::ReadLcdReg(uint32_t addr, uint8_t accessSize)
{
    uint8_t* bytePtr = &lcdRegisters_[addr - Memory::LCD_IO_ADDR_MIN];
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
    uint8_t* bytePtr = &lcdRegisters_[addr - Memory::LCD_IO_ADDR_MIN];

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
}
