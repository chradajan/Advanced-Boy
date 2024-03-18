#include <GUI/MainWindow.hpp>
#include <GUI/DebugWindow.hpp>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QtWidgets>

namespace GUI
{
MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent),
    debugWindow_(nullptr)
{
    InitializeMenuBar();
}

void MainWindow::InitializeMenuBar()
{
    // Set menu bar options
    fileMenu_ = menuBar()->addMenu("File");
    emulationMenu_ = menuBar()->addMenu("Emulation");
    optionsMenu_ = menuBar()->addMenu("Options");
    debugMenu_ = menuBar()->addMenu("Debug");

    // Set debug menu options
    auto debuggerAction = new QAction("Debugger", debugMenu_);
    QAction::connect(debuggerAction, &QAction::triggered, this, &MainWindow::OpenDebuggerWindow);
    debugMenu_->addAction(debuggerAction);
}

void MainWindow::OpenDebuggerWindow()
{
    if (!debugWindow_)
    {
        debugWindow_ = new DebugWindow();
    }

    debugWindow_->show();
}
}
