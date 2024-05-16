#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <MainWindow.hpp>
#include <QtCore/QObject>
#include <SDL2/SDL.h>

namespace fs = std::filesystem;
class MainWindow;

class EmuThread : public QObject
{
    Q_OBJECT

public:
    /// @brief Create the thread class used to run the emulator.
    /// @param romPath Path to game ROM.
    /// @param biosPath Path to GBA BIOS.
    /// @param logging Whether to log CPU instructions.
    /// @param mainWindow Reference to window where rendering takes place.
    EmuThread(fs::path romPath, fs::path biosPath, bool logging, MainWindow const& mainWindow);

    /// @brief Get the title of the currently loaded ROM.
    /// @return Title of ROM.
    std::string RomTitle() const;

    /// @brief Unpause the audio device, therefore resuming emulation.
    void Play();

    /// @brief Pause the audio device, therefore stopping emulation.
    void Pause();

signals:
    void RefreshScreen();

private:
    /// @brief Callback function used by the emulator to signal that the screen is ready to be refreshed.
    /// @param int Must take an integer argument to match function signature used by emulator events.
    void RefreshScreenCallback(int);

    bool gamePakSuccessfullyLoaded_;
    SDL_AudioDeviceID audioDevice_;
};
