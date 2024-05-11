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
#include <utility>
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
    backupType_(BackupType::None),
    eepromIndex_(0),
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

    // Setup backup media and load save file if present
    auto [backupType, backupSize] = DetectBackupType();
    backupType_ = backupType;

    if ((backupType != BackupType::None) && (backupSize > 0))
    {
        backupMedia_.resize(backupSize, 0xFF);
    }

    fs::path savePath = romPath;
    savePath.replace_extension("sav");

    if (fs::exists(savePath))
    {
        std::ifstream saveFile(savePath, std::ios::binary);

        if (!saveFile.fail())
        {
            switch (backupType)
            {
                case BackupType::SRAM:
                {
                    if (fs::file_size(savePath) == backupMedia_.size())
                    {
                        saveFile.read(reinterpret_cast<char*>(backupMedia_.data()), backupMedia_.size());
                    }

                    break;
                }
                case BackupType::EEPROM:
                {
                    size_t eepromSize = fs::file_size(savePath);

                    if ((eepromSize == 512) || (eepromSize == 8 * KiB))
                    {
                        backupMedia_.resize(eepromSize);
                        saveFile.read(reinterpret_cast<char*>(backupMedia_.data()), backupMedia_.size());
                    }

                    break;
                }
                case BackupType::FLASH:
                    break;
                default:
                    break;
            }
        }
    }

    romLoaded_ = true;
}

GamePak::~GamePak()
{
    if (!romPath_.empty() && !backupMedia_.empty())
    {
        fs::path savePath = romPath_;
        savePath.replace_extension("sav");
        std::ofstream saveFile(savePath, std::ios::binary);

        if (!saveFile.fail() && !backupMedia_.empty())
        {
            saveFile.write(reinterpret_cast<char*>(backupMedia_.data()), backupMedia_.size());
        }
    }
}

std::tuple<uint32_t, int, bool> GamePak::ReadGamePak(uint32_t addr, AccessSize alignment)
{
    addr = AlignAddress(addr, alignment);
    int accessTime = AccessTime(addr, false, alignment);

    if (EepromAccess(addr))
    {
        return {1, accessTime, false};
    }
    else if (SramAccess(addr))
    {
        return {ReadSRAM(addr, alignment), accessTime, false};
    }
    else if (FlashAccess(addr))
    {
        return {0, accessTime, false};
    }

    size_t index = addr % MAX_ROM_SIZE;

    if ((addr > ROM_ADDR_MAX) || ((index + static_cast<uint8_t>(alignment)) > ROM_.size()))
    {
        return {0, 1, true};
    }

    uint8_t* bytePtr = &ROM_[index];
    uint32_t value = ReadPointer(bytePtr, alignment);
    return {value, 1, false};  // TODO: Implement prefetch buffer and actual timing
}

int GamePak::WriteGamePak(uint32_t addr, uint32_t value, AccessSize alignment)
{
    int accessTime = AccessTime(addr, false, alignment);

    if (SramAccess(addr))
    {
        WriteSRAM(addr, value, alignment);
        return accessTime;
    }

    return 1;
}

std::pair<uint32_t, int> GamePak::ReadWAITCNT(uint32_t addr, AccessSize alignment)
{
    uint8_t* bytePtr = reinterpret_cast<uint8_t*>(&waitStateControl_.word_);
    uint8_t offset = addr & 0x03;
    uint32_t value = 0;

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

bool GamePak::EepromAccess(uint32_t addr) const
{
    if (backupType_ != BackupType::EEPROM)
    {
        return false;
    }

    if (ROM_.size() > 16 * MiB)
    {
        return (EEPROM_ADDR_LARGE_CART_MIN <= addr) && (addr <= EEPROM_ADDR_MAX);
    }

    return (EEPROM_ADDR_SMALL_CART_MIN <= addr) && (addr <= EEPROM_ADDR_MAX);
}

bool GamePak::SramAccess(uint32_t addr) const
{
    return (backupType_ == BackupType::SRAM) && (SRAM_FLASH_ADDR_MIN <= addr) && (addr <= SRAM_FLASH_ADDR_MAX);
}

bool GamePak::FlashAccess(uint32_t addr) const
{
    return (backupType_ == BackupType::FLASH) && (SRAM_FLASH_ADDR_MIN <= addr) && (addr <= SRAM_FLASH_ADDR_MAX);
}

void GamePak::SetEepromIndex(size_t index, int indexLength)
{
    if (backupType_ != BackupType::EEPROM)
    {
        return;
    }

    if (backupMedia_.empty())
    {
        if (indexLength == 6)
        {
            backupMedia_.resize(512, 0xFF);
        }
        else if (indexLength == 14)
        {
            backupMedia_.resize(8 * KiB, 0xFF);
        }
    }

    eepromIndex_ = index & 0x03FF;
}

uint64_t GamePak::GetEepromDoubleWord()
{
    if ((backupType_ != BackupType::EEPROM) || backupMedia_.empty() || ((eepromIndex_ * 8) >= backupMedia_.size()))
    {
        return 0xFFFF'FFFF'FFFF'FFFF;
    }

    uint64_t* eepromPtr = reinterpret_cast<uint64_t*>(backupMedia_.data());
    return eepromPtr[eepromIndex_];
}

void GamePak::WriteToEeprom(size_t index, int indexLength, uint64_t doubleWord)
{
    if (backupType_ != BackupType::EEPROM)
    {
        return;
    }

    if (backupMedia_.empty())
    {
        if (indexLength == 6)
        {
            backupMedia_.resize(512, 0xFF);
        }
        else if (indexLength == 14)
        {
            backupMedia_.resize(8 * KiB, 0xFF);
        }
    }

    index &= 0x03FF;

    if ((index * 8) < backupMedia_.size())
    {
        uint64_t* eepromPtr = reinterpret_cast<uint64_t*>(backupMedia_.data());
        eepromPtr[index] = doubleWord;
    }
}

int GamePak::AccessTime(uint32_t addr, bool sequential, AccessSize alignment) const
{
    uint8_t page = (addr & 0x0F00'0000) >> 24;
    int firstAccess = 0;
    int secondAccess = 0;

    switch (page)
    {
        case 0x08 ... 0x09:
        {
            firstAccess = 1 + (sequential ? SequentialWaitStates[0][waitStateControl_.flags_.waitState0SecondAccess] :
                                            NonSequentialWaitStates[waitStateControl_.flags_.waitState0FirstAccess]);

            if (alignment == AccessSize::WORD)
            {
                secondAccess = 1 + SequentialWaitStates[0][waitStateControl_.flags_.waitState0SecondAccess];
            }

            break;
        }
        case 0x0A ... 0x0B:
        {
            firstAccess = 1 + (sequential ? SequentialWaitStates[1][waitStateControl_.flags_.waitState1SecondAccess] :
                                            NonSequentialWaitStates[waitStateControl_.flags_.waitState1FirstAccess]);

            if (alignment == AccessSize::WORD)
            {
                secondAccess = 1 + SequentialWaitStates[1][waitStateControl_.flags_.waitState1SecondAccess];
            }

            break;
        }
        case 0x0C ... 0x0D:
        {
            firstAccess = 1 + (sequential ? SequentialWaitStates[2][waitStateControl_.flags_.waitState2SecondAccess] :
                                            NonSequentialWaitStates[waitStateControl_.flags_.waitState2FirstAccess]);

            if (alignment == AccessSize::WORD)
            {
                secondAccess = 1 + SequentialWaitStates[2][waitStateControl_.flags_.waitState2SecondAccess];
            }

            break;
        }
        case 0x0E:
            firstAccess = 1 + NonSequentialWaitStates[waitStateControl_.flags_.sramWaitCtrl];
            break;
        default:
            firstAccess = 1;
            secondAccess = (alignment == AccessSize::WORD) ? 1 : 0;
            break;
    }

    return firstAccess + secondAccess;
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

uint32_t GamePak::ReadSRAM(uint32_t addr, AccessSize alignment)
{
    addr = AlignAddress(addr, alignment);
    size_t index = (addr - SRAM_FLASH_ADDR_MIN) % backupMedia_.size();
    uint8_t* bytePtr = &backupMedia_[index];
    uint32_t value = ReadPointer(bytePtr, AccessSize::BYTE);

    if (alignment == AccessSize::HALFWORD)
    {
        value *= 0x0101;
    }
    else if (alignment == AccessSize::WORD)
    {
        value *= 0x0101'0101;
    }

    return value;
}

void GamePak::WriteSRAM(uint32_t addr, uint32_t value, AccessSize alignment)
{
    addr = AlignAddress(addr, alignment);
    size_t index = (addr - SRAM_FLASH_ADDR_MIN) % backupMedia_.size();
    uint8_t* bytePtr = &backupMedia_[index];

    if (alignment != AccessSize::BYTE)
    {
        value = std::rotr(value, (addr & 0x03) * 8) & MAX_U8;
        alignment = AccessSize::BYTE;
    }

    WritePointer(bytePtr, value, alignment);
}

std::pair<BackupType, size_t> GamePak::DetectBackupType()
{
    for (size_t i = 0; (i + 11) < ROM_.size(); i += 4)
    {
        char* start = reinterpret_cast<char*>(&ROM_[i]);
        std::string idString;
        idString.assign(start, 12);

        if (idString.starts_with("EEPROM_V"))
        {
            return {BackupType::EEPROM, 0};
        }
        else if (idString.starts_with("SRAM_V"))
        {
            return {BackupType::SRAM, 32 * KiB};
        }
        else if (idString.starts_with("FLASH"))
        {
            if (idString.starts_with("FLASH_V") || idString.starts_with("FLASH512_V"))
            {
                return {BackupType::FLASH, 64 * KiB};
            }
            else if (idString.starts_with("FLASH1M_V"))
            {
                return {BackupType::FLASH, 128 * KiB};
            }
        }
    }

    return {BackupType::None, 0};
}
}
