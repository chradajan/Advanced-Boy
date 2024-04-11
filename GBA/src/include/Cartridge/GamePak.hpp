#pragma once

#include <MemoryMap.hpp>
#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace Cartridge
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
    /// @return Value from specified address and number of cycles taken to read.
    std::pair<uint32_t, int> ReadMemory(uint32_t addr, uint8_t accessSize);

    /// @brief Write to memory.
    /// @param addr Aligned address.
    /// @param value Value to write to specified address.
    /// @param accessSize 1 = Byte, 2 = Halfword, 4 = Word.
    /// @return Number of cycles taken to write.
    int WriteMemory(uint32_t addr, uint32_t value, uint8_t accessSize);

    /// @brief Get value of WAITCNT register.
    /// @return Value of WAITCNT and number of cycles to read it.
    std::pair<uint32_t, int> ReadWAITCNT() { return {WAITCNT_, 1}; }

    /// @brief Set value of WAITCNT register.
    /// @param value Value to set WAITCNT to.
    /// @return Number of cycles to write it.
    int WriteWAITCNT(uint32_t value) { WAITCNT_ = value; return 1; }

private:
    /// @brief Determine which region of memory an address points to.
    /// @param addr Address to determine mapping of.
    /// @param accessSize Size in bytes of access.
    /// @param isWrite True if writing memory, false if reading.
    /// @return Pointer to mapped location in memory.
    uint8_t* GetPointerToMem(uint32_t addr, uint8_t accessSize, bool isWrite);

    // ROM Info
    bool romLoaded_;
    std::string romTitle_;

    // Memory
    std::vector<uint8_t> ROM_;
    std::array<uint8_t, 64 * KiB> SRAM_;

    // Registers
    uint16_t WAITCNT_;
};
}
