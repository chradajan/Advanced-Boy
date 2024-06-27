#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <QtCore/QThread>
#include <SDL2/SDL.h>

namespace fs = std::filesystem;

class EmuThread : public QThread
{
    Q_OBJECT

public:
    /// @brief Create the thread class used to run the emulator.
    /// @param biosPath Path to GBA BIOS.
    /// @param parent Pointer to parent object.
    EmuThread(fs::path biosPath, QObject* parent = nullptr);

    /// @brief Insert a cartridge. Can not be performed while emulation is running.
    /// @param romPath Path to ROM to load.
    void LoadROM(fs::path romPath);

    /// @brief Stop the emulation and audio threads, and power off GBA.
    void Quit();

    /// @brief Get the title of the currently loaded ROM.
    /// @return Title of ROM.
    std::string RomTitle() const;

    /// @brief Unlock and unpause the audio device.
    void StartAudioCallback();

    /// @brief Lock and pause the audio device.
    void PauseAudioCallback();

private:
    /// @brief Main emulation loop. Access by calling start().
    void run();

    bool gamePakSuccessfullyLoaded_;
    SDL_AudioDeviceID audioDevice_;
};
