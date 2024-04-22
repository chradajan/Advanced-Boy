#pragma once

#include <QtCore/QtCore>
#include <QtCore/QTimer>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QtWidgets>
#include <cstdint>
#include <filesystem>
#include <set>
#include <string>

namespace fs = std::filesystem;
class EmuThread;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /// @brief Initialize the main GUI window.
    /// @param romPath Path to game ROM.
    /// @param biosPath Path to GBA BIOS.
    /// @param logging Whether to log CPU instructions.
    /// @param parent Parent widget.
    MainWindow(fs::path romPath, fs::path biosPath, bool logging, QWidget* parent = nullptr);

    ~MainWindow();

public slots:
    void RefreshScreen();

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

    /// @brief Update the GBA gamepad based on which keys are currently pressed.
    void SendKeyPresses() const;

    /// @brief Update the window title every second with the latest FPS.
    void UpdateWindowTitle();

    // Emulator
    EmuThread* gbaThread;
    std::string romTitle_;
    int frameCounter_;
    QTimer frameTimer_;

    // Menu bar drop downs
    QMenu* fileMenu_;
    QMenu* emulationMenu_;
    QMenu* optionsMenu_;

    // Display
    QLabel lcd_;
    int screenScale_;

    // Gamepad
    std::set<int> pressedKeys_;
};
