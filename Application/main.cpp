#include <AdvancedBoy.hpp>
#include <MainWindow.hpp>
#include <EmuThread.hpp>
#include <QtCore/QtCore>
#include <QtWidgets/QApplication>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    // Until GUI is fully implemented, grab arguments from command line
    fs::path romPath = "";
    fs::path biosPath = "";
    bool logging = false;

    for (int i = 1; i < argc; ++i)
    {
        switch (i)
        {
            case 1:
                romPath = argv[1];
                break;
            case 2:
                biosPath = argv[2];
                break;
            case 3:
            {
                std::string logStr = argv[3];
                logging = (logStr != "0");
                break;
            }
        }
    }

    MainWindow mainWindow(romPath, biosPath, logging);
    mainWindow.show();
    return app.exec();
}
