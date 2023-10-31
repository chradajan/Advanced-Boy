#pragma once

#include <cstdint>

namespace Memory
{
// Useful constants
constexpr uint32_t KiB = 1024;
constexpr uint32_t MiB = KiB * KiB;
constexpr uint32_t MAX_ROM_SIZE = 32 * MiB;

// Memory map constants
constexpr uint32_t BIOS_ADDR_MIN = 0x00000000;
constexpr uint32_t BIOS_ADDR_MAX = 0x00003FFF;

constexpr uint32_t WRAM_ON_BOARD_ADDR_MIN = 0x02000000;
constexpr uint32_t WRAM_ON_BOARD_ADDR_MAX = 0x0203FFFF;

constexpr uint32_t WRAM_ON_CHIP_ADDR_MIN = 0x03000000;
constexpr uint32_t WRAM_ON_CHIP_ADDR_MAX = 0x03007FFF;

constexpr uint32_t IO_REG_ADDR_MIN = 0x04000000;
constexpr uint32_t IO_REG_ADDR_MAX = 0x040003FE;

constexpr uint32_t PALETTE_RAM_ADDR_MIN = 0x05000000;
constexpr uint32_t PALETTE_RAM_ADDR_MAX = 0x050003FF;

constexpr uint32_t VRAM_ADDR_MIN = 0x06000000;
constexpr uint32_t VRAM_ADDR_MAX = 0x06017FFF;

constexpr uint32_t OAM_ADDR_MIN = 0x07000000;
constexpr uint32_t OAM_ADDR_MAX = 0x070003FF;

constexpr uint32_t GAME_PAK_ROM_ADDR_MIN = 0x08000000;
constexpr uint32_t GAME_PAK_ROM_ADDR_MAX = 0x0DFFFFFF;

constexpr uint32_t GAME_PAK_SRAM_ADDR_MIN = 0x0E000000;
constexpr uint32_t GAME_PAK_SRAM_ADDR_MAX = 0x0E00FFFF;
}
