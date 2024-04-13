#pragma once

#include <QtCore/QtCore>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QtWidgets>
#include <cstdint>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /// @brief Initialize the main GUI window.
    /// @param parent Parent widget.
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow() = default;

public slots:
    void RefreshScreen();

private:
    /// @brief Initialize the window menu bar.
    void InitializeMenuBar();

    /// @brief Initialize the central widget used to display LCD output.
    void InitializeLCD();

    /// @brief Resize the window to a fixed size based on window scale property.
    void ResizeWindow();

    // Menu bar drop downs
    QMenu* fileMenu_;
    QMenu* emulationMenu_;
    QMenu* optionsMenu_;

    // Display
    QLabel lcd_;
    int screenScale_;
};
