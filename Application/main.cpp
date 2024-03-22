#include <AdvancedBoy.hpp>

int main(int argc, char** argv)
{
    Initialize("");

    if (argc > 1)
    {
        InsertCartridge(argv[1]);
    }

    PowerOn();

    return 0;
}
