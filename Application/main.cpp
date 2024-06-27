#include <filesystem>
#include <string>
#include <AdvancedBoy.hpp>
#include <MainWindow.hpp>
#include <EmuThread.hpp>
#include <SDL2/SDL.h>
#include <QtCore/QtCore>
#include <QtWidgets/QApplication>

#define main SDL_main
namespace fs = std::filesystem;

int main(int argv, char** args)
{
    QApplication app(argv, args);
    MainWindow mainWindow;
    mainWindow.show();
    return app.exec();
}
