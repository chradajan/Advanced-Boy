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

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent),
    gbaThread_("", this),
    romTitle_("Advanced Boy"),
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
    setWindowTitle("Advanced Boy");

    refreshScreenTimer_.setTimerType(Qt::PreciseTimer);
    connect(&refreshScreenTimer_, &QTimer::timeout, this, &RefreshScreen);
    refreshScreenTimer_.start(16);

    connect(&fpsTimer_, &QTimer::timeout, this, &UpdateWindowTitle);
    fpsTimer_.start(1000);
}

void MainWindow::InitializeMenuBar()
{
    fileMenu_ = menuBar()->addMenu("File");
    emulationMenu_ = menuBar()->addMenu("Emulation");
    optionsMenu_ = menuBar()->addMenu("Options");

    QAction* loadRomAction = new QAction("Load ROM...", this);
    connect(loadRomAction, &QAction::triggered, this, &SelectROM);
    fileMenu_->addAction(loadRomAction);
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

        if (event->key() == 72)  // H
        {
            ::ToggleCpuLogging();
        }

        if (event->key() == 71)  // G
        {
            ::ToggleSystemLogging();
        }
    }
}

void MainWindow::closeEvent(QCloseEvent*)
{
    if (gbaThread_.isRunning())
    {
        gbaThread_.PauseAudioCallback();
        gbaThread_.requestInterruption();
        gbaThread_.wait();
    }

    gbaThread_.Quit();
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent* event)
{
    fs::path romPath = "";

    for (auto& url : event->mimeData()->urls())
    {
        if (url.isLocalFile())
        {
            romPath = url.toLocalFile().toStdString();

            if (romPath.has_extension() && (romPath.extension() == ".gba"))
            {
                break;
            }
        }
    }

    LoadROM(romPath);
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

    ::UpdateGamepad(gamepad);
}

void MainWindow::UpdateWindowTitle()
{
    int frames = ::GetAndResetFrameCounter();
    std::string newTitle = std::format("{} ({} fps)", romTitle_, frames);
    setWindowTitle(QString::fromStdString(newTitle));
}

void MainWindow::RefreshScreen()
{
    uint8_t* frameBuffer = ::GetRawFrameBuffer();

    if (frameBuffer != nullptr)
    {
        SendKeyPresses();
        auto image = QImage(frameBuffer, 240, 160, QImage::Format_RGB555);
        image.rgbSwap();
        lcd_.setPixmap(QPixmap::fromImage(image).scaled(lcd_.width(), lcd_.height()));
    }
}

void MainWindow::SelectROM()
{
    auto romSelectWindow = QFileDialog(this);
    romSelectWindow.setFileMode(QFileDialog::FileMode::ExistingFile);
    fs::path romPath = romSelectWindow.getOpenFileName(this, "Select ROM...", QString(), "GBA (*.gba)").toStdString();
    LoadROM(romPath);
}

void MainWindow::LoadROM(fs::path romPath)
{
    if (fs::exists(fs::path(romPath)))
    {
        if (gbaThread_.isRunning())
        {
            gbaThread_.PauseAudioCallback();
            gbaThread_.requestInterruption();
            gbaThread_.wait();
        }

        gbaThread_.LoadROM(romPath);
        romTitle_ = gbaThread_.RomTitle();
        setWindowTitle(QString::fromStdString(romTitle_));
        gbaThread_.start();
        gbaThread_.StartAudioCallback();
    }
}
