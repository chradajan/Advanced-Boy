#pragma once

#include <Memory/Memory.hpp>
#include <Memory/GamePak.hpp>
#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <utility>

namespace fs = std::filesystem;

namespace Memory
{
class MemoryManager
{
public:
    /// @brief Initialize the memory manager.
    /// @param biosPath Path to GBA BIOS file.
    MemoryManager(fs::path biosPath);

    MemoryManager() = delete;
    MemoryManager(MemoryManager const&) = delete;
    MemoryManager(MemoryManager&&) = delete;
    MemoryManager& operator=(MemoryManager const&) = delete;
    MemoryManager& operator=(MemoryManager&&) = delete;
    ~MemoryManager() = default;

    /// @brief Load a Game Pak into memory.
    /// @param romPath Path to ROM file.
    /// @return Whether the GamePak was valid and successfully loaded.
    bool LoadGamePak(fs::path romPath);

    // TODO Implement actual memory access times. For now just return 1.

    /// @brief Read from memory.
    /// @param addr Aligned address.
    /// @param accessSize 1 = Byte, 2 = Halfword, 4 = Word.
    /// @return Value from specified address and number of cycles taken to read.
    std::pair<uint32_t, int> ReadMemory(uint32_t addr, uint8_t accessSize);

    /// @brief Write to memory.
    /// @param addr Aligned address.
    /// @param val Value to write to specified address.
    /// @param accessSize 1 = Byte, 2 = Halfword, 4 = Word.
    /// @return Number of cycles taken to write.
    int WriteMemory(uint32_t addr, uint32_t val, uint8_t accessSize);

private:
    /// @brief Set all internal memory to 0.
    void ZeroMemory();

    /// @brief Determine which region of memory an address points to.
    /// @param addr Address to determine mapping of.
    /// @param accessSize Size in bytes of access.
    /// @return Pointer to mapped location in memory.
    uint8_t* GetPointerToMem(uint32_t addr, uint8_t accessSize);

    // General Internal Memory
    std::array<uint8_t, 16  * KiB> BIOS_;           // 00000000-00003FFF    BIOS - System ROM
    std::array<uint8_t, 256 * KiB> onBoardWRAM_;    // 02000000-0203FFFF    WRAM - On-board Work RAM
    std::array<uint8_t, 32  * KiB> onChipWRAM_;     // 03000000-03007FFF    WRAM - On-chip Work RAM

    // Internal Display Memory
    std::array<uint8_t, 1  * KiB> paletteRAM_;  // 05000000-050003FF    BG/OBJ Palette RAM
    std::array<uint8_t, 96 * KiB> VRAM_;        // 06000000-06017FFF    VRAM - Video RAM
    std::array<uint8_t, 1  * KiB> OAM_;         // 07000000-070003FF    OAM - OBJ Attributes

    // Game Pak
    // 04000000-0400005F
    std::unique_ptr<GamePak> gamePak_;

    /* Not sure yet what the best way to implement I/O register handling is. For now, keep all I/O registers in a single r/w array.

    // LCD I/O Registers
    // 04000000-0400005F
    std::array<uint8_t, 0x60> lcdRegisters_;

    // Sound Registers
    // 04000060-040000AF
    std::array<uint8_t, 0x50> soundRegisters_;

    // DMA Transfer Channels Registers
    // 040000B0 - 04000FF
    std::array<uint8_t, 0x50> dmaTransferChannelsRegisters_;

    // Timer Registers
    //04000100 - 0400011F
    std::array<uint8_t, 0x20> timerRegisters_;

    // Serial Communication (1) Registers
    // 04000120 - 0400012F
    std::array<uint8_t, 0x10> serialCommunication1Registers_;

    // Keypad Input Registers
    // 04000130 - 04000133
    std::array<uint8_t, 4> keypadInputRegisters_;

    // Serial Communication (2) Registers
    // 04000134 - 040001FF
    std::array<uint8_t, 0xCC> serialCommunication2Registers_;

    // Interrupt, Waitstate, and Power-Down Control Registers
    // 04000200 - 04000804
    // TODO: Pretty wasteful to use 1.5kb for only a couple registers.
    std::array<uint8_t, 0x604> interruptWaitstatePowerDownControlRegisters_;

    */
   std::array<uint8_t, 0x804> placeholderIoRegisters_;
};
}
