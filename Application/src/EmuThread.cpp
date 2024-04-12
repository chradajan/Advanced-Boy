#include <EmuThread.hpp>
#include <AdvancedBoy.hpp>
#include <MainWindow.hpp>
#include <QtCore/QtCore>
#include <cstdint>
#include <functional>

EmuThread::EmuThread(int argc, char** argv, MainWindow const& mainWindow)
{
    Initialize("", std::bind(&RefreshScreenCallback, this, std::placeholders::_1));

    QObject::connect(this, &RefreshScreen,
                     &mainWindow, &MainWindow::RefreshScreen);

    gamePakSuccessfullyLoaded_ = false;

    if (argc > 1)
    {
        gamePakSuccessfullyLoaded_ = InsertCartridge(argv[1]);
    }
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
