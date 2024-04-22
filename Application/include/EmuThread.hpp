#pragma once

#include <MainWindow.hpp>
#include <QtCore/QThread>
#include <cstdint>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;
class MainWindow;

class EmuThread : public QThread
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

    /// @brief Run the GBA indefinitely.
    void run();

signals:
    void RefreshScreen();

private:
    /// @brief Callback function used by the emulator to signal that the screen is ready to be refreshed.
    /// @param int Must take an integer argument to match function signature used by emulator events.
    void RefreshScreenCallback(int);

    bool gamePakSuccessfullyLoaded_;
};
