#include <Memory/MemoryManager.hpp>
#include <Memory/GamePak.hpp>
#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <sstream>

namespace Memory
{
MemoryManager::MemoryManager(fs::path const biosPath)
{
    // TODO
    // Fill BIOS with 0 for now until actual BIOS loading is implemented.
    (void)biosPath;
    BIOS_.fill(0);

    // Initialize internal memory with 0.
    ZeroMemory();

    // Initialize with no Game Pak loaded.
    gamePak_ = nullptr;
}

void MemoryManager::LoadGamePak(fs::path const romPath)
{
    gamePak_.reset();
    gamePak_ = std::make_unique<GamePak>(romPath);

    ZeroMemory();
}

uint32_t MemoryManager::ReadMemory(uint32_t const addr, uint8_t const accessSize)
{
    if ((addr >= IO_REG_ADDR_MIN) && (addr <= IO_REG_ADDR_MAX))
    {
        // TODO
        std::stringstream exceptionMsg;
        exceptionMsg << "IO Read. Addr: " << (unsigned int)addr << " Size: " << (unsigned int)accessSize;
        throw std::runtime_error(exceptionMsg.str());
    }
    else if ((addr >= GAME_PAK_ROM_ADDR_MIN) && (addr <= GAME_PAK_SRAM_ADDR_MAX))
    {
        return gamePak_->ReadMemory(addr, accessSize);
    }
    else
    {
        auto bytePtr = GetPointerToMem(addr, accessSize);

        switch (accessSize)
        {
            case 1:
                return *bytePtr;
            case 2:
                return *reinterpret_cast<uint16_t*>(bytePtr);
            case 4:
                return *reinterpret_cast<uint32_t*>(bytePtr);
            default:
                throw std::runtime_error("Illegal Read Memory access size");
        }
    }
}

void MemoryManager::WriteMemory(uint32_t const addr, uint32_t const val, uint8_t const accessSize)
{
    if ((addr >= IO_REG_ADDR_MIN) && (addr <= IO_REG_ADDR_MAX))
    {
        // TODO
        std::stringstream exceptionMsg;
        exceptionMsg << "IO Write. Addr: " << (unsigned int)addr << " Val: " << (unsigned int)val << " Size: " << (unsigned int)accessSize;
        throw std::runtime_error(exceptionMsg.str());
    }
    else if ((addr >= GAME_PAK_ROM_ADDR_MIN) && (addr <= GAME_PAK_SRAM_ADDR_MAX))
    {
        gamePak_->WriteMemory(addr, val, accessSize);
    }
    else
    {
        auto bytePtr = GetPointerToMem(addr, accessSize);

        switch (accessSize)
        {
            case 1:
                *bytePtr = val;
                break;
            case 2:
                *reinterpret_cast<uint16_t*>(bytePtr) = val;
                break;
            case 4:
                *reinterpret_cast<uint32_t*>(bytePtr) = val;
                break;
            default:
                throw std::runtime_error("Illegal Write Memory access size");
        }
    }
}

void MemoryManager::ZeroMemory()
{
    onBoardWRAM_.fill(0);
    onChipWRAM_.fill(0);
    paletteRAM_.fill(0);
    VRAM_.fill(0);
    OAM_.fill(0);
}

uint8_t* MemoryManager::GetPointerToMem(uint32_t const addr, uint8_t const accessSize)
{
    uint8_t* mappedPtr = nullptr;
    size_t adjustedIndex = 0;
    size_t maxIndex = 0;

    if (addr <= BIOS_ADDR_MAX)
    {
        adjustedIndex = addr;
        maxIndex = BIOS_ADDR_MAX;
        mappedPtr = &BIOS_[adjustedIndex];
    }
    else if ((addr >= WRAM_ON_BOARD_ADDR_MIN) && (addr <= WRAM_ON_BOARD_ADDR_MAX))
    {
        adjustedIndex = addr - WRAM_ON_BOARD_ADDR_MIN;
        maxIndex = WRAM_ON_BOARD_ADDR_MAX;
        mappedPtr = &onBoardWRAM_[adjustedIndex];
    }
    else if ((addr >= WRAM_ON_CHIP_ADDR_MIN) && (addr <= WRAM_ON_CHIP_ADDR_MAX))
    {
        adjustedIndex = addr - WRAM_ON_CHIP_ADDR_MIN;
        maxIndex = WRAM_ON_CHIP_ADDR_MAX;
        mappedPtr = &onChipWRAM_[adjustedIndex];
    }
    else if ((addr >= IO_REG_ADDR_MIN) && (addr <= IO_REG_ADDR_MAX))
    {
        // TODO
    }
    else if ((addr >= PALETTE_RAM_ADDR_MIN) && (addr <= PALETTE_RAM_ADDR_MAX))
    {
        adjustedIndex = addr - PALETTE_RAM_ADDR_MIN;
        maxIndex = PALETTE_RAM_ADDR_MAX;
        mappedPtr = &paletteRAM_[adjustedIndex];
    }
    else if ((addr >= VRAM_ADDR_MIN) && (addr <= VRAM_ADDR_MAX))
    {
        adjustedIndex = addr - VRAM_ADDR_MIN;
        maxIndex = VRAM_ADDR_MAX;
        mappedPtr = &VRAM_[adjustedIndex];
    }
    else if ((addr >= OAM_ADDR_MIN) && (addr <= OAM_ADDR_MAX))
    {
        adjustedIndex = addr - OAM_ADDR_MIN;
        maxIndex = OAM_ADDR_MAX;
        mappedPtr = &OAM_[adjustedIndex];
    }

    if ((adjustedIndex + accessSize - 1) > maxIndex)
    {
        std::stringstream exceptionMsg;
        exceptionMsg << "Illegal memory access. Addr: " << (unsigned int)addr << " Size: " << (unsigned int)accessSize;
        throw std::runtime_error(exceptionMsg.str());
    }
    else if (!mappedPtr)
    {
        std::stringstream exceptionMsg;
        exceptionMsg << "Illegal memory access. Could not map address: " << (unsigned int)addr << " size: " << (unsigned int)accessSize;
        throw std::runtime_error(exceptionMsg.str());
    }

    return mappedPtr;
}
}