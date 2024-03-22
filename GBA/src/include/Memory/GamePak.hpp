#pragma once

#include <Memory/Memory.hpp>
#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace Memory
{
class GamePak
{
public:
    /// @brief Initialize a Game Pak.
    /// @param romPath Path to GBA ROM file.
    GamePak(fs::path romPath);

    /// @brief Destructor that handles creating save file upon unloading a Game Pak.
    ~GamePak();

    GamePak() = delete;
    GamePak(GamePak const&) = delete;
    GamePak(GamePak&&) = delete;
    GamePak& operator=(GamePak const&) = delete;
    GamePak& operator=(GamePak&&) = delete;

    /// @brief Check if a valid ROM file is loaded.
    /// @return True if ROM is loaded and ready to be accessed, false otherwise.
    bool RomLoaded() const { return romLoaded_; };

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
    /// @brief Determine which region of memory an address points to.
    /// @param addr Address to determine mapping of.
    /// @param accessSize Size in bytes of access.
    /// @param isWrite True if writing memory, false if reading.
    /// @return Pointer to mapped location in memory.
    uint8_t* GetPointerToMem(uint32_t addr, uint8_t accessSize, bool isWrite);

    bool romLoaded_;
    std::string romTitle_;

    std::vector<uint8_t> ROM_;
    std::array<uint8_t, 64 * KiB> SRAM_;
};
}
