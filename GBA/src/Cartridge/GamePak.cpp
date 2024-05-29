#include <Cartridge/GamePak.hpp>
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include <Cartridge/EEPROM.hpp>
#include <Cartridge/Flash.hpp>
#include <Cartridge/SRAM.hpp>
#include <System/MemoryMap.hpp>
#include <System/SystemControl.hpp>
#include <Utilities/MemoryUtilities.hpp>

namespace Cartridge
{
GamePak::GamePak(fs::path const romPath) :
    romLoaded_(false),
    romTitle_(""),
    romPath_(romPath),
    backupType_(BackupType::None),
    eeprom_(nullptr),
    flash_(nullptr),
    sram_(nullptr)
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
    size_t backupSizeInBytes = 0;
    std::tie(backupType_, backupSizeInBytes) = DetectBackupType();
    fs::path savePath = romPath;
    savePath.replace_extension("sav");

    switch (backupType_)
    {
        case BackupType::None:
            break;
        case BackupType::SRAM:
            sram_ = std::make_unique<SRAM>(savePath);
            break;
        case BackupType::EEPROM:
            eeprom_ = std::make_unique<EEPROM>(savePath);
            break;
        case BackupType::FLASH:
            flash_ = std::make_unique<Flash>(savePath, backupSizeInBytes);
            break;
    }

    romLoaded_ = true;
}

void GamePak::Reset()
{
    lastAddrRead_ = 0;

    if (eeprom_ != nullptr)
    {
        eeprom_->Reset();
    }
    else if (flash_ != nullptr)
    {
        flash_->Reset();
    }
}

std::tuple<uint32_t, int, bool> GamePak::ReadGamePak(uint32_t addr, AccessSize alignment)
{
    uint32_t value = 0;
    int cycles = 1;
    bool openBus = false;

    if (EepromAccess(addr))
    {
        std::tie(value, cycles) = eeprom_->Read(addr, alignment);
    }
    else if (SramAccess(addr))
    {
        std::tie(value, cycles) = sram_->Read(addr, alignment);
    }
    else if (FlashAccess(addr))
    {
        std::tie(value, cycles) = flash_->Read(addr, alignment);
    }
    else
    {
        std::tie(value, cycles, openBus) = ReadROM(addr, alignment);
    }

    return {value, cycles, openBus};
}

int GamePak::WriteGamePak(uint32_t addr, uint32_t value, AccessSize alignment)
{
    int cycles = 1;

    if (SramAccess(addr))
    {
        cycles = sram_->Write(addr, value, alignment);
    }
    else if (FlashAccess(addr))
    {
        flash_->Write(addr, value, alignment);
    }

    return cycles;
}

bool GamePak::EepromAccess(uint32_t addr) const
{
    if (backupType_ != BackupType::EEPROM)
    {
        return false;
    }

    if (ROM_.size() > (16 * MiB))
    {
        return (EEPROM_ADDR_LARGE_CART_MIN <= addr) && (addr <= EEPROM_ADDR_MAX);
    }

    return (EEPROM_ADDR_SMALL_CART_MIN <= addr) && (addr <= EEPROM_ADDR_MAX);
}

bool GamePak::SramAccess(uint32_t addr) const
{
    return (backupType_ == BackupType::SRAM) && (SRAM_ADDR_MIN <= addr) && (addr <= SRAM_ADDR_MAX);
}

bool GamePak::FlashAccess(uint32_t addr) const
{
    return (backupType_ == BackupType::FLASH) && (SRAM_ADDR_MIN <= addr) && (addr <= SRAM_ADDR_MAX);
}

int GamePak::SetEepromIndex(size_t index, int indexLength)
{
    int cycles = 0;

    if (backupType_ == BackupType::EEPROM)
    {
        cycles = eeprom_->SetIndex(index, indexLength);
    }

    return cycles;
}

std::pair<uint64_t, int> GamePak::ReadFromEeprom()
{
    uint64_t value = MAX_U64;
    int cycles = 0;

    if (backupType_ == BackupType::EEPROM)
    {
        std::tie(value, cycles) = eeprom_->ReadDoubleWord();
    }

    return {value, cycles};
}

int GamePak::WriteToEeprom(size_t index, int indexLength, uint64_t value)
{
    int cycles = 0;

    if (backupType_ == BackupType::EEPROM)
    {
        cycles = eeprom_->WriteDoubleWord(index, indexLength, value);
    }

    return cycles;
}

std::tuple<uint32_t, int, bool> GamePak::ReadROM(uint32_t addr, AccessSize alignment)
{
    uint32_t value = 0;
    int cycles = 1;
    auto region = WaitState::ZERO;
    uint8_t page = (addr & 0x0F00'0000) >> 24;

    switch (page)
    {
        case 0x08 ... 0x09:
            region = WaitState::ZERO;
            break;
        case 0x0A ... 0x0B:
            region = WaitState::ONE;
            addr -= MAX_ROM_SIZE;
            break;
        case 0x0C ... 0x0D:
            region = WaitState::TWO;
            addr -= (2 * MAX_ROM_SIZE);
            break;
        default:
            return {value, cycles, true};
    }

    size_t index = addr - GAME_PAK_ADDR_MIN;

    if (index >= ROM_.size())
    {
        return {value, cycles, true};
    }

    (void)region;
    // TODO: Use region and prefetch to determine actual cycle count.

    uint8_t* bytePtr = &ROM_[index];
    value = ReadPointer(bytePtr, alignment);
    return {value, cycles, false};
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
