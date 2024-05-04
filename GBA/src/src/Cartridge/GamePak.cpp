#include <Cartridge/GamePak.hpp>
#include <array>
#include <bit>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>
#include <System/MemoryMap.hpp>
#include <System/Utilities.hpp>

static constexpr int NonSequentialWaitStates[4] = {4, 3, 2, 8};
static constexpr int SequentialWaitStates[3][2] = { {2, 1}, {4, 1}, {8, 1} };

namespace Cartridge
{
GamePak::GamePak(fs::path const romPath) :
    romLoaded_(false),
    romTitle_(""),
    romPath_(romPath),
    waitStateControl_(0)
{
    if (romPath.empty())
    {
        return;
    }

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

    // Check for save file
    fs::path savePath = romPath;
    savePath.replace_extension("sav");

    if (fs::exists(savePath))
    {
        std::ifstream saveFile(savePath, std::ios::binary);

        if (!saveFile.fail())
        {
            saveFile.read(reinterpret_cast<char*>(SRAM_.data()), SRAM_.size());
        }
    }

    romTitle_ = titleStream.str();
    romLoaded_ = true;
}

GamePak::~GamePak()
{
    if (!romPath_.empty())
    {
        fs::path savePath = romPath_;
        savePath.replace_extension("sav");
        std::ofstream saveFile(savePath, std::ios::binary);

        if (!saveFile.fail())
        {
            saveFile.write(reinterpret_cast<char*>(SRAM_.data()), SRAM_.size());
        }
    }
}

std::tuple<uint32_t, int, bool> GamePak::ReadROM(uint32_t addr, AccessSize alignment)
{
    addr = AlignAddress(addr, alignment);
    int waitState = WaitStateRegion(addr);
    int cycles = WaitStateCycles(waitState);
    size_t index = addr % MAX_ROM_SIZE;

    if (index >= ROM_.size())
    {
        return {0, cycles, true};
    }
    else if (index + static_cast<uint8_t>(alignment) > ROM_.size())
    {
        return {0, cycles, false};
    }

    uint8_t* bytePtr = &(ROM_.at(index));
    uint32_t value = ReadPointer(bytePtr, alignment);
    return {value, cycles, false};
}

std::pair<uint32_t, int> GamePak::ReadSRAM(uint32_t addr, AccessSize alignment)
{
    if (addr > GAME_PAK_SRAM_ADDR_MAX)
    {
        addr = GAME_PAK_SRAM_ADDR_MIN + (addr % (64 * KiB));
    }

    addr = AlignAddress(addr, alignment);
    size_t index = addr - GAME_PAK_SRAM_ADDR_MIN;
    uint8_t* bytePtr = &(SRAM_.at(index));
    uint32_t value = ReadPointer(bytePtr, AccessSize::BYTE);
    int cycles = WaitStateCycles(3);

    if (alignment == AccessSize::HALFWORD)
    {
        value *= 0x0101;
    }
    else if (alignment == AccessSize::WORD)
    {
        value *= 0x0101'0101;
    }

    return {value, cycles};
}

int GamePak::WriteSRAM(uint32_t addr, uint32_t value, AccessSize alignment)
{
    if (addr > GAME_PAK_SRAM_ADDR_MAX)
    {
        addr = GAME_PAK_SRAM_ADDR_MIN + (addr % (64 * KiB));
    }

    int cycles = WaitStateCycles(3);
    uint32_t alignedAddr = AlignAddress(addr, alignment);
    size_t index = alignedAddr - GAME_PAK_SRAM_ADDR_MIN;
    uint8_t* bytePtr = &(SRAM_.at(index));

    if (alignment != AccessSize::BYTE)
    {
        value = std::rotr(value, (addr & 0x03) * 8) & MAX_U8;
        alignment = AccessSize::BYTE;
    }

    WritePointer(bytePtr, value, alignment);
    return cycles;
}

std::pair<uint32_t, int> GamePak::ReadWAITCNT(uint32_t addr, AccessSize alignment)
{
    uint8_t* bytePtr = reinterpret_cast<uint8_t*>(&waitStateControl_.word_);
    uint8_t offset = addr & 0x03;
    uint32_t value;

    switch (alignment)
    {
        case AccessSize::BYTE:
            value = bytePtr[offset];
            break;
        case AccessSize::HALFWORD:
            value = *reinterpret_cast<uint16_t*>(&bytePtr[offset]);
            break;
        case AccessSize::WORD:
            value = *reinterpret_cast<uint32_t*>(&bytePtr[offset]);
            break;
    }

    return {value, 1};
}

int GamePak::NonSequentialAccessTime() const
{
    return 1 + NonSequentialWaitStates[waitStateControl_.flags_.waitState0FirstAccess];
}

int GamePak::SequentialAccessTime(uint32_t addr) const
{
    int waitStateRegion = WaitStateRegion(addr);
    int waitStateIndex = (waitStateControl_.word_ >> (4 + (3 * waitStateRegion))) & 0x01;
    return 1 + SequentialWaitStates[waitStateRegion][waitStateIndex];
}

int GamePak::WaitStateRegion(uint32_t addr) const
{
    uint8_t page = (addr & 0x0F00'0000) >> 24;
    int waitState = 0;

    switch (page)
    {
        case 0x08 ... 0x09:
            waitState = 0;
            break;
        case 0x0A ... 0x0B:
            waitState = 1;
            break;
        case 0x0C ... 0x0D:
            waitState = 2;
            break;
        default:
            waitState = 0;
            break;
            //throw std::runtime_error(std::format("Illegal wait state region. Addr: {:08X}, Page: {:02X}", addr, page));
    }

    return waitState;
}

int GamePak::WriteWAITCNT(uint32_t addr, uint32_t value, AccessSize alignment)
{
    uint8_t* bytePtr = reinterpret_cast<uint8_t*>(&waitStateControl_.word_);
    uint8_t offset = addr & 0x03;

    switch (alignment)
    {
        case AccessSize::BYTE:
            bytePtr[offset] = value;
            break;
        case AccessSize::HALFWORD:
            *reinterpret_cast<uint16_t*>(&bytePtr[offset]) = value;
            break;
        case AccessSize::WORD:
            *reinterpret_cast<uint32_t*>(&bytePtr[offset]) = value;
            break;
    }

    return 1;
}
}
