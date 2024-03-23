#include <System/GameBoyAdvance.hpp>
#include <Logging/Logging.hpp>
#include <Memory/MemoryManager.hpp>
#include <filesystem>
#include <functional>

GameBoyAdvance::GameBoyAdvance(fs::path const biosPath) :
    memMgr_(biosPath),
    cpu_(std::bind(&Memory::MemoryManager::ReadMemory,  &memMgr_, std::placeholders::_1, std::placeholders::_2),
         std::bind(&Memory::MemoryManager::WriteMemory, &memMgr_, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)),
    biosLoaded_(biosPath != ""),
    gamePakLoaded_(false)
{
    Logging::InitializeLogging();
}

bool GameBoyAdvance::LoadGamePak(fs::path const romPath)
{
    gamePakLoaded_ = memMgr_.LoadGamePak(romPath);
    return gamePakLoaded_;
}

void GameBoyAdvance::Run()
{
    if (!biosLoaded_ && !gamePakLoaded_)
    {
        return;
    }

    while (true)
    {
        cpu_.Clock();
    }
}
