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

    /// @brief Read from memory.
    /// @param addr Aligned address.
    /// @param accessSize 1 = Byte, 2 = Halfword, 4 = Word.
    /// @return Value from specified address.
    uint32_t ReadMemory(uint32_t addr, uint8_t accessSize);

    /// @brief Write to memory.
    /// @param addr Aligned address.
    /// @param val Value to write to specified address.
    /// @param accessSize 1 = Byte, 2 = Halfword, 4 = Word.
    void WriteMemory(uint32_t addr, uint32_t val, uint8_t accessSize);

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
    std::unique_ptr<GamePak> gamePak_;
};
}
