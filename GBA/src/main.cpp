#include <GameBoyAdvance.hpp>
#include <GUI/MainWindow.hpp>
#include <SDL.h>
#include <QtWidgets/QApplication>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    GUI::MainWindow mainWindow;
    mainWindow.show();
    app.exec();

    // GameBoyAdvance gba("");

    // if (argc > 1)
    // {
    //     gba.LoadGamePak(argv[1]);
    // }

    // gba.Run();

    return 0;
}
