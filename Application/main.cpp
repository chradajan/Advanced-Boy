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

    // Until GUI is fully implemented, grab arguments from command line
    fs::path romPath = "";
    fs::path biosPath = "";
    bool logging = false;

    for (int i = 1; i < argv; ++i)
    {
        switch (i)
        {
            case 1:
                romPath = args[1];
                break;
            case 2:
                biosPath = args[2];
                break;
            case 3:
            {
                std::string logStr = args[3];
                logging = (logStr != "0");
                break;
            }
        }
    }

    MainWindow mainWindow(romPath, biosPath, logging);
    mainWindow.show();
    return app.exec();
}
