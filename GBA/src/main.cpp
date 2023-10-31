#include <GameBoyAdvance.hpp>

int main(int argc, char** argv)
{
    GameBoyAdvance gba("");

    if (argc > 1)
    {
        gba.LoadGamePak(argv[1]);
    }

    return 0;
}
