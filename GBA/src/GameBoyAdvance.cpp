#include <GameBoyAdvance.hpp>
#include <filesystem>

GameBoyAdvance::GameBoyAdvance(fs::path const biosPath) :
    memMgr_(biosPath)
{
}

void GameBoyAdvance::LoadGamePak(fs::path const romPath)
{
    memMgr_.LoadGamePak(romPath);
}
