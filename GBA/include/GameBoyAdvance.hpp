#pragma once

#include <ARM7TDMI/ARM7TDMI.hpp>
#include <Memory/MemoryManager.hpp>
#include <filesystem>

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
    void LoadGamePak(fs::path romPath);

    /// @brief Run GBA indefinitely.
    void Run();

private:
    Memory::MemoryManager memMgr_;
    CPU::ARM7TDMI cpu_;
};
