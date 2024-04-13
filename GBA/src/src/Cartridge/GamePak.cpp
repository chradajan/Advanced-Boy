#include <Cartridge/GamePak.hpp>
#include <MemoryMap.hpp>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Cartridge
{
GamePak::GamePak(fs::path const romPath) :
    WAITCNT_(0)
{
    romLoaded_ = false;

    // Load ROM data into memory.
    auto const fileSizeInBytes = fs::file_size(romPath);
    ROM_.resize(fileSizeInBytes);
    std::ifstream rom(romPath, std::ios::binary);

    if (rom.fail())
    {
        return;
    }

    rom.read(reinterpret_cast<char*>(ROM_.data()), fileSizeInBytes);

    // Read game title from cartridge header.
    std::stringstream titleStream;

    for (size_t charIndex = 0; charIndex < 12; ++charIndex)
    {
        uint8_t titleChar = ROM_.at(0x00A0 + charIndex);

        if (titleChar == 0)
        {
            continue;
        }

        titleStream << static_cast<char>(titleChar);
    }

    romTitle_ = titleStream.str();
    romLoaded_ = true;
}

GamePak::~GamePak()
{
    // TODO
}

std::pair<uint32_t, int> GamePak::ReadMemory(uint32_t addr, uint8_t accessSize)
{
    // TODO Implement actual access times
    auto bytePtr = GetPointerToMem(addr, accessSize, false);

    switch (accessSize)
    {
        case 1:
            return {*bytePtr, 1};
        case 2:
            return {*reinterpret_cast<uint16_t*>(bytePtr), 1};
        case 4:
            return {*reinterpret_cast<uint32_t*>(bytePtr), 1};
        default:
            throw std::runtime_error("Illegal Read ROM Memory access size");
    }
}

int GamePak::WriteMemory(uint32_t addr, uint32_t val, uint8_t accessSize)
{
    // TODO Implement actual access times
    auto bytePtr = GetPointerToMem(addr, accessSize, false);

    if (bytePtr)
    {
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
                throw std::runtime_error("Illegal Write SRAM Memory access size");
        }
    }

    return 1;
}

uint8_t* GamePak::GetPointerToMem(uint32_t const addr, uint8_t const accessSize, bool const isWrite)
{
    size_t adjustedIndex = 0;

    if (addr <= GAME_PAK_ROM_ADDR_MAX)
    {
        if (isWrite)
        {
            return nullptr;
        }

        adjustedIndex = addr % MAX_ROM_SIZE;

        if ((adjustedIndex + accessSize - 1) > ROM_.size())
        {
            std::stringstream exceptionMsg;
            exceptionMsg << "Illegal ROM memory access. Addr: " << (unsigned int)addr << " Size: " << (unsigned int)accessSize;
            throw std::runtime_error(exceptionMsg.str());
        }

        return &ROM_.at(adjustedIndex);
    }

    adjustedIndex = addr - GAME_PAK_SRAM_ADDR_MIN;

    if ((adjustedIndex + accessSize - 1) > SRAM_.size())
    {
        std::stringstream exceptionMsg;
        exceptionMsg << "Illegal SRAM access. Addr: " << (unsigned int)addr << " Size: " << (unsigned int)accessSize;
        throw std::runtime_error(exceptionMsg.str());
    }

    return &SRAM_.at(adjustedIndex);
}
}
