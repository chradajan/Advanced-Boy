#include <EmuThread.hpp>
#include <AdvancedBoy.hpp>
#include <MainWindow.hpp>
#include <QtCore/QtCore>
#include <cstdint>
#include <functional>
#include <string>

EmuThread::EmuThread(fs::path romPath, fs::path biosPath, bool logging, MainWindow const& mainWindow)
{
    Initialize(biosPath, std::bind(&RefreshScreenCallback, this, 0));

    QObject::connect(this, &RefreshScreen,
                     &mainWindow, &MainWindow::RefreshScreen);

    gamePakSuccessfullyLoaded_ = InsertCartridge(romPath);
    EnableLogging(logging);
}

void EmuThread::PowerOff()
{
    ::PowerOff();
}

std::string EmuThread::RomTitle() const
{
    return ::RomTitle();
}

void EmuThread::run()
{
    if (gamePakSuccessfullyLoaded_)
    {
        PowerOn();
    }
}

void EmuThread::RefreshScreenCallback(int)
{
    emit RefreshScreen();
}
