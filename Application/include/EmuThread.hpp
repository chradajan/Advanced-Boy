#pragma once

#include <MainWindow.hpp>
#include <QtCore/QThread>
#include <cstdint>

class EmuThread : public QThread
{
    Q_OBJECT

public:

    /// @brief Create the thread class used to run the emulator.
    /// @param argc Number of args passed to program.
    /// @param argv Arguments passed to program.
    /// @param mainWindow Reference to window where rendering takes place.
    EmuThread(int argc, char** argv, MainWindow const& mainWindow);

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
