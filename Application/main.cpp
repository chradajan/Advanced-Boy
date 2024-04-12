#include <AdvancedBoy.hpp>
#include <MainWindow.hpp>
#include <EmuThread.hpp>
#include <QtCore/QtCore>
#include <QtWidgets/QApplication>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    MainWindow mainWindow;
    EmuThread* gbaThread = new EmuThread(argc, argv, mainWindow);

    gbaThread->start();
    mainWindow.show();
    app.exec();

    delete gbaThread;

    return 0;
}
