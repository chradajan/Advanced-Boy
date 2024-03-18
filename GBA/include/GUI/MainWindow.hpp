#pragma once

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QtWidgets>

namespace GUI
{
class DebugWindow;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow() = default;

private:
    void InitializeMenuBar();

    void OpenDebuggerWindow();

    QMenu* fileMenu_;
    QMenu* emulationMenu_;
    QMenu* optionsMenu_;
    QMenu* debugMenu_;

    DebugWindow* debugWindow_;
};
}
