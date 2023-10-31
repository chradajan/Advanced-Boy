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

private:
    bool romLoaded_;
    std::string romTitle_;

    std::vector<uint8_t> ROM_;
    std::array<uint8_t, 64 * KiB> SRAM_;
};
}
