#include <Cartridge/GamePak.hpp>
#include <System/MemoryMap.hpp>
#include <System/Utilities.hpp>
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

namespace Cartridge
{
GamePak::GamePak(fs::path const romPath) :
    romLoaded_(false),
    romTitle_(""),
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

    romTitle_ = titleStream.str();
    romLoaded_ = true;
}

GamePak::~GamePak()
{
    // TODO
}

std::tuple<uint32_t, int, bool> GamePak::ReadROM(uint32_t addr, AccessSize alignment)
{
    addr = AlignAddress(addr, alignment);
    uint8_t page = (addr & 0xFF00'0000) >> 24;
    uint8_t waitState;

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
            throw std::runtime_error(std::format("Illegal wait state region on ROM read. Addr: {:08X}, Page: {:02X}", addr,page));
    }

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
        addr = GAME_PAK_ROM_ADDR_MIN + (addr % (64 * KiB));
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
        addr = GAME_PAK_ROM_ADDR_MIN + (addr % (64 * KiB));
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
