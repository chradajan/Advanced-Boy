#include <MainWindow.hpp>
#include <AdvancedBoy.hpp>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QtWidgets>
#include <cstdint>

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent),
    lcd_(this)
{
    setMinimumSize(240, 160);
    InitializeMenuBar();
    InitializeLCD();
}

void MainWindow::InitializeMenuBar()
{
    fileMenu_ = menuBar()->addMenu("File");
    emulationMenu_ = menuBar()->addMenu("Emulation");
    optionsMenu_ = menuBar()->addMenu("Options");
}

void MainWindow::InitializeLCD()
{
    setCentralWidget(&lcd_);
}

void MainWindow::RefreshScreen()
{
    auto image = QImage(GetRawFrameBuffer(), 240, 160, QImage::Format_RGB888);
    lcd_.setPixmap(QPixmap::fromImage(image).scaled(240, 160));
}
