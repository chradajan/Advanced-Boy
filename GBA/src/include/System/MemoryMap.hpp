#pragma once

#include <cstdint>
#include <limits>

// Useful constants
constexpr uint32_t KiB = 1024;
constexpr uint32_t MiB = KiB * KiB;

enum class AccessSize : uint8_t
{
    BYTE = 1,
    HALFWORD = 2,
    WORD = 4
};

// Useful 32-bit constants
constexpr uint32_t MSB_32 = 0b1000'0000'0000'0000'0000'0000'0000'0000;
constexpr uint32_t MAX_U32 = std::numeric_limits<uint32_t>::max();

// Useful 16-bit constants
constexpr uint16_t MSB_16 = 0b1000'0000'0000'0000;
constexpr uint16_t MAX_U16 = std::numeric_limits<uint16_t>::max();

// Useful 8-bit constants
constexpr uint8_t MSB_8 = 0b1000'0000;
constexpr uint8_t MAX_U8 = std::numeric_limits<uint8_t>::max();

// Memory map constants
constexpr uint32_t BIOS_ADDR_MIN = 0x0000'0000;
constexpr uint32_t BIOS_ADDR_MAX = 0x0000'3FFF;

constexpr uint32_t WRAM_ON_BOARD_ADDR_MIN = 0x0200'0000;
constexpr uint32_t WRAM_ON_BOARD_ADDR_MAX = 0x0203'FFFF;

constexpr uint32_t WRAM_ON_CHIP_ADDR_MIN = 0x0300'0000;
constexpr uint32_t WRAM_ON_CHIP_ADDR_MAX = 0x0300'7FFF;

constexpr uint32_t IO_REG_ADDR_MIN = 0x0400'0000;
constexpr uint32_t IO_REG_ADDR_MAX = 0x0400'0803;

constexpr uint32_t PALETTE_RAM_ADDR_MIN = 0x0500'0000;
constexpr uint32_t PALETTE_RAM_ADDR_MAX = 0x0500'03FF;

constexpr uint32_t VRAM_ADDR_MIN = 0x0600'0000;
constexpr uint32_t VRAM_ADDR_MAX = 0x0601'7FFF;

constexpr uint32_t OAM_ADDR_MIN = 0x0700'0000;
constexpr uint32_t OAM_ADDR_MAX = 0x0700'03FF;

constexpr uint32_t GAME_PAK_ADDR_MIN = 0x0800'0000;
constexpr uint32_t GAME_PAK_ADDR_MAX = 0x0E00'FFFF;

// Sub I/O Ranges
constexpr uint32_t LCD_IO_ADDR_MIN = 0x0400'0000;
constexpr uint32_t LCD_IO_ADDR_MAX = 0x0400'005F;

constexpr uint32_t SOUND_IO_ADDR_MIN = 0x0400'0060;
constexpr uint32_t SOUND_IO_ADDR_MAX = 0x0400'00AF;

constexpr uint32_t DMA_TRANSFER_CHANNELS_IO_ADDR_MIN = 0x0400'00B0;
constexpr uint32_t DMA_TRANSFER_CHANNELS_IO_ADDR_MAX = 0x0400'00FF;

constexpr uint32_t TIMER_IO_ADDR_MIN = 0x0400'0100;
constexpr uint32_t TIMER_IO_ADDR_MAX = 0x0400'011F;

constexpr uint32_t SERIAL_COMMUNICATION_1_IO_ADDR_MIN = 0x0400'0120;
constexpr uint32_t SERIAL_COMMUNICATION_1_IO_ADDR_MAX = 0x0400'012F;

constexpr uint32_t KEYPAD_INPUT_IO_ADDR_MIN = 0x0400'0130;
constexpr uint32_t KEYPAD_INPUT_IO_ADDR_MAX = 0x0400'0133;

constexpr uint32_t SERIAL_COMMUNICATION_2_IO_ADDR_MIN = 0x0400'0134;
constexpr uint32_t SERIAL_COMMUNICATION_2_IO_ADDR_MAX = 0x0400'01FF;

constexpr uint32_t INT_WTST_PWRDWN_IO_ADDR_MIN = 0x0400'0200;
constexpr uint32_t INT_WTST_PWRDWN_IO_ADDR_MAX = 0x0400'0803;

constexpr uint32_t WAITCNT_ADDR = 0x0400'0204;
