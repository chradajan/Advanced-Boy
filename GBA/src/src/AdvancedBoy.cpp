#include <AdvancedBoy.hpp>
#include <Gamepad.hpp>
#include <Config.hpp>
#include <Logging/Logging.hpp>
#include <System/GameBoyAdvance.hpp>
#include <filesystem>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>

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

void FillAudioBuffer(int samples)
{
    if (!gba)
    {
        throw std::runtime_error("Ran uninitialized GBA");
    }

    gba->FillAudioBuffer(samples);
}

void DrainAudioBuffer(float* buffer)
{
    if (!gba)
    {
        throw std::runtime_error("Audio callback with uninitialized GBA");
    }

    gba->DrainAudioBuffer(buffer);
}

void UpdateGamepad(Gamepad gamepad)
{
    if (!gba)
    {
        throw std::runtime_error("Update gamepad of uninitialized GBA");
    }

    gba->UpdateGamepad(gamepad);
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

std::string RomTitle()
{
    if (!gba)
    {
        throw std::runtime_error("Got ROM title of uninitialized GBA");
    }

    return gba->RomTitle();
}

void PowerOff()
{
    if (gba)
    {
        gba.reset();
    }
}
