#include <AdvancedBoy.hpp>

int main(int argc, char** argv)
{
    Initialize("");
    bool gamePakSuccessfullyLoaded = false;

    if (argc > 1)
    {
        gamePakSuccessfullyLoaded = InsertCartridge(argv[1]);
    }

    if (gamePakSuccessfullyLoaded)
    {
        while (true)
        {
            PowerOn();
        }
    }

    return 0;
}
