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
    /// @param romPath Path to game ROM.
    /// @param biosPath Path to GBA BIOS.
    /// @param parent Pointer to parent object.
    EmuThread(fs::path romPath, fs::path biosPath, QObject* parent = nullptr);

    /// @brief Allow the main emulation loop to exit.
    void StopEmulation() { runEmulation_ = false; }

    /// @brief Stop the emulation and audio threads, and power off GBA.
    void Quit();

    /// @brief Get the title of the currently loaded ROM.
    /// @return Title of ROM.
    std::string RomTitle() const;

    /// @brief Unpause the audio device, therefore resuming emulation.
    void Play();

    /// @brief Pause the audio device, therefore stopping emulation.
    void Pause();

    /// @brief Main loop to run emulation.
    void run();

private:
    bool gamePakSuccessfullyLoaded_;
    volatile bool runEmulation_;
    SDL_AudioDeviceID audioDevice_;
};
