#include <GameBoyAdvance.hpp>
#include <Memory/MemoryManager.hpp>
#include <filesystem>
#include <functional>

GameBoyAdvance::GameBoyAdvance(fs::path const biosPath) :
    memMgr_(biosPath),
    cpu_(std::bind(&Memory::MemoryManager::ReadMemory,  &memMgr_, std::placeholders::_1, std::placeholders::_2),
         std::bind(&Memory::MemoryManager::WriteMemory, &memMgr_, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))
{
}

void GameBoyAdvance::LoadGamePak(fs::path const romPath)
{
    memMgr_.LoadGamePak(romPath);
}
