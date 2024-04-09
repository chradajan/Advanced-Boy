#pragma once

#include <Graphics/Registers.hpp>
#include <array>
#include <cstdint>
#include <utility>

namespace Graphics
{
class PPU
{
public:
    /// @brief Initialize the PPU.
    PPU();

    /// @brief Read a memory mapped LCD I/O register.
    /// @param addr Address of memory mapped register.
    /// @param accessSize 1 = Byte, 2 = Halfword, 4 = Word.
    /// @return Value of specified register and number of cycles taken to read.
    std::pair<uint32_t, int> ReadLcdReg(uint32_t addr, uint8_t accessSize);

    /// @brief Write a memory mapped LCD I/O register.
    /// @param addr Address of memory mapped register.
    /// @param val Value to write to register.
    /// @param accessSize 1 = Byte, 2 = Halfword, 4 = Word.
    /// @return Number of cycles taken to write.
    int WriteLcdReg(uint32_t addr, uint32_t val, uint8_t accessSize);

private:
    // Frame buffer
    uint8_t* frameBuffer_;

    // LCD I/O Registers (0400'0000h - 0400'005Fh)
    std::array<uint8_t, 0x60> lcdRegisters_;
    DISPCNT& lcdControl_;
    DISPSTAT& lcdStatus_;
    VCOUNT& verticalCounter_;
};
}
