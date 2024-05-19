#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <QtCore/QObject>
#include <SDL2/SDL.h>

namespace fs = std::filesystem;

class EmuThread : public QObject
{
    Q_OBJECT

public:
    /// @brief Create the thread class used to run the emulator.
    /// @param romPath Path to game ROM.
    /// @param biosPath Path to GBA BIOS.
    /// @param logging Whether to log CPU instructions.
    EmuThread(fs::path romPath, fs::path biosPath, bool logging);

    /// @brief Stop audio callbacks and dump logs.
    ~EmuThread();

    /// @brief Get the title of the currently loaded ROM.
    /// @return Title of ROM.
    std::string RomTitle() const;

    /// @brief Unpause the audio device, therefore resuming emulation.
    void Play();

    /// @brief Pause the audio device, therefore stopping emulation.
    void Pause();

    bool gamePakSuccessfullyLoaded_;
    SDL_AudioDeviceID audioDevice_;
};
