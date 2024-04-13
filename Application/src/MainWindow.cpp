#include <MainWindow.hpp>
#include <AdvancedBoy.hpp>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QtWidgets>
#include <cstdint>

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent),
    lcd_(this),
    screenScale_(4)
{
    ResizeWindow();
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

void MainWindow::ResizeWindow()
{
    int width = 240 * screenScale_;
    int lcdHeight = 160 * screenScale_;
    int windowHeight = menuBar()->height() + lcdHeight;
    setFixedSize(width, windowHeight);
    lcd_.resize(width, lcdHeight);
}

void MainWindow::RefreshScreen()
{
    auto image = QImage(GetRawFrameBuffer(), 240, 160, QImage::Format_RGB888);
    lcd_.setPixmap(QPixmap::fromImage(image).scaled(lcd_.width(), lcd_.height()));
}
