#include <Memory/MemoryManager.hpp>
#include <Memory/GamePak.hpp>
#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>

namespace Memory
{
MemoryManager::MemoryManager(fs::path const biosPath)
{
    // TODO
    // Fill BIOS with 0 for now until actual BIOS loading is implemented.
    (void)biosPath;
    BIOS_.fill(0);

    // Initialize internal memory with 0.
    ZeroMemory();

    // Initialize with no Game Pak loaded.
    gamePak_ = nullptr;
}

void MemoryManager::LoadGamePak(fs::path const romPath)
{
    gamePak_.reset();
    gamePak_ = std::make_unique<GamePak>(romPath);

    ZeroMemory();
}

void MemoryManager::ZeroMemory()
{
    onBoardWRAM_.fill(0);
    onChipWRAM_.fill(0);
    paletteRAM_.fill(0);
    VRAM_.fill(0);
    OAM_.fill(0);
}
}