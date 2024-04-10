#pragma once

#include <ARM7TDMI/ARM7TDMI.hpp>
#include <Cartridge/GamePak.hpp>
#include <Graphics/PPU.hpp>
#include <MemoryMap.hpp>
#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>

namespace fs = std::filesystem;

class GameBoyAdvance
{
public:
    /// @brief Initialize the Game Boy Advance.
    /// @param biosPath Path to GBA BIOS file.
    GameBoyAdvance(fs::path biosPath);

    GameBoyAdvance() = delete;
    GameBoyAdvance(GameBoyAdvance const&) = delete;
    GameBoyAdvance(GameBoyAdvance&&) = delete;
    GameBoyAdvance& operator=(GameBoyAdvance const&) = delete;
    GameBoyAdvance& operator=(GameBoyAdvance&&) = delete;
    ~GameBoyAdvance() = default;

    /// @brief Load a Game Pak into memory.
    /// @param romPath Path to ROM file.
    /// @return Whether the GamePak was valid and successfully loaded.
    bool LoadGamePak(fs::path romPath);

    /// @brief Access the raw frame buffer data.
    /// @return Raw pointer to frame buffer.
    uint8_t* GetRawFrameBuffer() { return ppu_.GetRawFrameBuffer(); }

    /// @brief Run GBA indefinitely.
    void Run();

private:
    /// @brief Set all internal memory to 0.
    void ZeroMemory();

    /// @brief Determine which region of memory an address points to.
    /// @param addr Address to determine mapping of.
    /// @param accessSize Size in bytes of access.
    /// @return Pointer to mapped location in memory and number of cycles to access it.
    std::pair<uint8_t*, int> GetPointerToMem(uint32_t addr, uint8_t accessSize);

    /// @brief Read from memory.
    /// @param addr Aligned address.
    /// @param accessSize 1 = Byte, 2 = Halfword, 4 = Word.
    /// @return Value from specified address and number of cycles taken to read.
    std::pair<uint32_t, int> ReadMemory(uint32_t addr, int accessSize);

    /// @brief Write to memory.
    /// @param addr Aligned address.
    /// @param value Value to write to specified address.
    /// @param accessSize 1 = Byte, 2 = Halfword, 4 = Word.
    /// @return Number of cycles taken to write.
    int WriteMemory(uint32_t addr, uint32_t value, int accessSize);

    /// @brief Read a memory mapped I/O register.
    /// @param addr Address of memory mapped register.
    /// @param accessSize 1 = Byte, 2 = Halfword, 4 = Word.
    /// @return Value of specified register and number of cycles taken to read.
    std::pair<uint32_t, int> ReadIoReg(uint32_t addr, int accessSize);

    /// @brief Write a memory mapped I/O register.
    /// @param addr Address of memory mapped register.
    /// @param value Value to write to specified register.
    /// @param accessSize 1 = Byte, 2 = Halfword, 4 = Word.
    /// @return Number of cycles taken to write.
    int WriteIoReg(uint32_t addr, uint32_t value, int accessSize);

    // Components
    CPU::ARM7TDMI cpu_;
    Graphics::PPU ppu_;
    std::unique_ptr<Cartridge::GamePak> gamePak_;

    // Memory
    std::array<uint8_t,  16 * KiB> BIOS_;           // 00000000-00003FFF    BIOS - System ROM
    std::array<uint8_t, 256 * KiB> onBoardWRAM_;    // 02000000-0203FFFF    WRAM - On-board Work RAM
    std::array<uint8_t,  32 * KiB> onChipWRAM_;     // 03000000-03007FFF    WRAM - On-chip Work RAM
    std::array<uint8_t,   1 * KiB> paletteRAM_;     // 05000000-050003FF    BG/OBJ Palette RAM
    std::array<uint8_t,  96 * KiB> VRAM_;           // 06000000-06017FFF    VRAM - Video RAM
    std::array<uint8_t,   1 * KiB> OAM_;            // 07000000-070003FF    OAM - OBJ Attributes

    std::array<uint8_t, 0x804> placeholderIoRegisters_;

    bool biosLoaded_;
    bool gamePakLoaded_;
    int ppuCatchupCycles_;
};
