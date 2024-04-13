#include <AdvancedBoy.hpp>
#include <Config.hpp>
#include <Logging/Logging.hpp>
#include <System/GameBoyAdvance.hpp>
#include <filesystem>
#include <functional>
#include <memory>
#include <stdexcept>

namespace fs = std::filesystem;

// Globals
std::unique_ptr<GameBoyAdvance> gba;
bool gamePakLoaded;

// Public header definitions

void Initialize(fs::path biosPath, std::function<void(int)> refreshScreenCallback)
{
    gba.reset();
    gba = std::make_unique<GameBoyAdvance>(biosPath, refreshScreenCallback);
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

void EnableLogging(bool enable)
{
    Config::LOGGING_ENABLED = enable;

    if (enable)
    {
        Logging::LogMgr.Initialize();
    }
}
