#pragma once

#include <cstdint>
#include <filesystem>
#include <set>
#include <string>
#include <EmuThread.hpp>
#include <QtCore/QtCore>
#include <QtCore/QTimer>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QtWidgets>

namespace fs = std::filesystem;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /// @brief Initialize the main GUI window.
    /// @param parent Parent widget.
    MainWindow(QWidget* parent = nullptr);

private:
    /// @brief Initialize the window menu bar.
    void InitializeMenuBar();

    /// @brief Initialize the central widget used to display LCD output.
    void InitializeLCD();

    /// @brief Resize the window to a fixed size based on window scale property.
    void ResizeWindow();

    /// @brief Add whichever key was pressed to the set of currently pressed keys.
    /// @param event Key press event.
    void keyPressEvent(QKeyEvent* event);

    /// @brief Remove whichever key was released from the set of currently pressed keys.
    /// @param event Key release event.
    void keyReleaseEvent(QKeyEvent* event);

    /// @brief Handle main window closing event.
    void closeEvent(QCloseEvent*);

    /// @brief Event handling when file is dragged into window.
    /// @param event Drag event.
    void dragEnterEvent(QDragEnterEvent* event);

    /// @brief Load any dropped GBA ROMs.
    /// @param event Drop event.
    void dropEvent(QDropEvent* event);

    /// @brief Update the GBA gamepad based on which keys are currently pressed.
    void SendKeyPresses() const;

    /// @brief Update the window title every second with the latest FPS.
    void UpdateWindowTitle();

    /// @brief Update the LCD label widget.
    void RefreshScreen();

    /// @brief Open a file explorer to allow user to select a ROM to load.
    void SelectROM();

    /// @brief Load a ROM and update window title.
    /// @param romPath Path of ROM to load.
    void LoadROM(fs::path romPath);

    // Emulator
    EmuThread gbaThread_;
    std::string romTitle_;
    QTimer fpsTimer_;

    // Menu bar drop downs
    QMenu* fileMenu_;
    QMenu* emulationMenu_;
    QMenu* optionsMenu_;

    // Display
    QLabel lcd_;
    QTimer refreshScreenTimer_;
    int screenScale_;

    // Gamepad
    std::set<int> pressedKeys_;
};
