#include <AdvancedBoy.hpp>
#include <System/GameBoyAdvance.hpp>
#include <filesystem>
#include <memory>
#include <stdexcept>

namespace fs = std::filesystem;

// Globals
std::unique_ptr<GameBoyAdvance> gba;
bool gamePakLoaded;

// Public header definitions

void Initialize(fs::path biosPath)
{
    gba.reset();
    gba = std::make_unique<GameBoyAdvance>(biosPath);
}

bool InsertCartridge(fs::path romPath)
{
    if (!gba)
    {
        throw std::runtime_error("Inserted cartridge into uninitialized GBA");
    }

    gamePakLoaded = gba->LoadGamePak(romPath);
    return gamePakLoaded;
}

void PowerOn()
{
    if (!gba || !gamePakLoaded)
    {
        throw std::runtime_error("Attempted to run uninitialized GBA");
    }

    gba->Run();
}

uint8_t* GetRawFrameBuffer()
{
    if (!gba)
    {
        throw std::runtime_error("Grabbed frame buffer of uninitialized GBA");
    }

    return gba->GetRawFrameBuffer();
}
