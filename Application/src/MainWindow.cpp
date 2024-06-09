#include <MainWindow.hpp>
#include <cstdint>
#include <filesystem>
#include <format>
#include <set>
#include <string>
#include <AdvancedBoy.hpp>
#include <EmuThread.hpp>
#include <Gamepad.hpp>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QtWidgets>

namespace fs = std::filesystem;

MainWindow::MainWindow(fs::path romPath, fs::path biosPath, bool logging, QWidget* parent) :
    QMainWindow(parent),
    gbaThread_(romPath, biosPath, logging, this),
    fpsTimer_(this),
    lcd_(this),
    refreshScreenTimer_(this),
    screenScale_(4),
    pressedKeys_()
{
    ResizeWindow();
    InitializeMenuBar();
    InitializeLCD();
    setAttribute(Qt::WA_QuitOnClose);
    romTitle_ = gbaThread_.RomTitle();
    setWindowTitle(QString::fromStdString(romTitle_));

    gbaThread_.Play();
    gbaThread_.start();

    refreshScreenTimer_.setTimerType(Qt::PreciseTimer);
    connect(&refreshScreenTimer_, &QTimer::timeout, this, &RefreshScreen);
    refreshScreenTimer_.start(10);

    connect(&fpsTimer_, &QTimer::timeout, this, &UpdateWindowTitle);
    fpsTimer_.start(1000);
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

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    if (event)
    {
        pressedKeys_.insert(event->key());
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent* event)
{
    if (event)
    {
        pressedKeys_.erase(event->key());
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    gbaThread_.Quit();
    QMainWindow::closeEvent(event);
}

void MainWindow::SendKeyPresses() const
{
    Gamepad gamepad;

    // Hard code gamepad bindings for now
    // WASD     -> Directions keys
    // A        -> L
    // B        -> K
    // Start    -> Return
    // Select   -> Backspace
    // L        -> Q
    // R        -> E

    if (pressedKeys_.contains(87)) gamepad.buttons_.Up = 0;
    if (pressedKeys_.contains(65)) gamepad.buttons_.Left = 0;
    if (pressedKeys_.contains(83)) gamepad.buttons_.Down = 0;
    if (pressedKeys_.contains(68)) gamepad.buttons_.Right = 0;

    if (pressedKeys_.contains(16777220)) gamepad.buttons_.Start = 0;
    if (pressedKeys_.contains(16777219)) gamepad.buttons_.Select = 0;

    if (pressedKeys_.contains(76)) gamepad.buttons_.A = 0;
    if (pressedKeys_.contains(75)) gamepad.buttons_.B = 0;

    if (pressedKeys_.contains(81)) gamepad.buttons_.L = 0;
    if (pressedKeys_.contains(69)) gamepad.buttons_.R = 0;

    UpdateGamepad(gamepad);
}

void MainWindow::UpdateWindowTitle()
{
    int frames = ::GetAndResetFrameCounter();
    std::string newTitle = std::format("{} ({} fps)", romTitle_, frames);
    setWindowTitle(QString::fromStdString(newTitle));
}

void MainWindow::RefreshScreen()
{
    SendKeyPresses();
    auto image = QImage(::GetRawFrameBuffer(), 240, 160, QImage::Format_RGB555);
    image.rgbSwap();
    lcd_.setPixmap(QPixmap::fromImage(image).scaled(lcd_.width(), lcd_.height()));
}
