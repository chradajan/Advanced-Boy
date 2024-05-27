#pragma once

#include <cstdint>

// Memory region pages
enum class MemoryPage : uint8_t
{
    BIOS = 0x00,
    WRAM_SLOW = 0x02,
    WRAM_FAST = 0x03,
    IO_REG = 0x04,
    PRAM = 0x05,
    VRAM = 0x06,
    OAM = 0x07,
    GAMEPAK_MIN = 0x08,
    GAMEPAK_MAX = 0x0F,
    INVALID = 0xFF
};

// Memory region addresses
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
constexpr uint32_t LCD_IO_ADDR_MAX = 0x0400'0057;

constexpr uint32_t SOUND_IO_ADDR_MIN = 0x0400'0060;
constexpr uint32_t SOUND_IO_ADDR_MAX = 0x0400'00AB;

constexpr uint32_t DMA_TRANSFER_CHANNELS_IO_ADDR_MIN = 0x0400'00B0;
constexpr uint32_t DMA_TRANSFER_CHANNELS_IO_ADDR_MAX = 0x0400'00E3;

constexpr uint32_t TIMER_IO_ADDR_MIN = 0x0400'0100;
constexpr uint32_t TIMER_IO_ADDR_MAX = 0x0400'0113;

constexpr uint32_t SERIAL_COMMUNICATION_1_IO_ADDR_MIN = 0x0400'0120;
constexpr uint32_t SERIAL_COMMUNICATION_1_IO_ADDR_MAX = 0x0400'012F;

constexpr uint32_t KEYPAD_INPUT_IO_ADDR_MIN = 0x0400'0130;
constexpr uint32_t KEYPAD_INPUT_IO_ADDR_MAX = 0x0400'0133;

constexpr uint32_t SERIAL_COMMUNICATION_2_IO_ADDR_MIN = 0x0400'0134;
constexpr uint32_t SERIAL_COMMUNICATION_2_IO_ADDR_MAX = 0x0400'015B;

constexpr uint32_t INT_WTST_PWRDWN_IO_ADDR_MIN = 0x0400'0200;
constexpr uint32_t INT_WTST_PWRDWN_IO_ADDR_MAX = 0x0400'0803;

constexpr uint32_t WAITCNT_ADDR = 0x0400'0204;

// Interrupt, Waitstate, and Power-Down Control

constexpr uint32_t INT_WAITCNT_ADDR_MIN = 0x0400'0200;
constexpr uint32_t INT_WAITCNT_ADDR_MAX = 0x0400'020B;

constexpr uint32_t POSTFLG_HALTCNT_ADDR_MIN = 0x0400'0300;
constexpr uint32_t POSTFLG_HALTCNT_ADDR_MAX = 0x0400'0303;

constexpr uint32_t UNDOCUMENTED_ADDR_MIN = 0x0400'0410;
constexpr uint32_t UNDOCUMENTED_ADDR_MAX = 0x0400'0413;

constexpr uint32_t INTERNAL_MEM_CONTROL_ADDR_MIN = 0x0400'0800;
constexpr uint32_t INTERNAL_MEM_CONTROL_ADDR_MAX = 0x0400'0803;
