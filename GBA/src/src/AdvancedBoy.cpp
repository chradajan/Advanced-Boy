#include <AdvancedBoy.hpp>
#include <System/GameBoyAdvance.hpp>
#include <filesystem>
#include <memory>
#include <stdexcept>

namespace fs = std::filesystem;

// Globals
std::unique_ptr<GameBoyAdvance> gba;

// Public header definitions

void Initialize(fs::path biosPath, fs::path romPath)
{
    gba.reset();
    gba = std::make_unique<GameBoyAdvance>(biosPath);

    if (!romPath.empty())
    {
        gba->LoadGamePak(romPath);
    }
}

void InsertCartridge(fs::path romPath)
{
    if (!gba)
    {
        throw std::runtime_error("Inserted cartridge into uninitialized GBA");
    }

    gba->LoadGamePak(romPath);
}

void PowerOn()
{
    if (!gba)
    {
        throw std::runtime_error("Powered on uninitialized GBA");
    }

    gba->Run();
}
