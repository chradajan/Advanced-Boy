#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include <Cartridge/EEPROM.hpp>
#include <Cartridge/Flash.hpp>
#include <Cartridge/SRAM.hpp>
#include <Utilities/MemoryUtilities.hpp>

namespace fs = std::filesystem;

namespace Cartridge
{
constexpr uint32_t MAX_ROM_SIZE = 32 * MiB;

enum class BackupType
{
    None,
    SRAM,
    EEPROM,
    FLASH
};

class GamePak
{
public:
    /// @brief Initialize a Game Pak.
    /// @param romPath Path to GBA ROM file.
    GamePak(fs::path romPath);

    /// @brief Reset the GamePak to its power-up state.
    void Reset();

    /// @brief Check if a valid ROM file is loaded.
    /// @return True if ROM is loaded and ready to be accessed, false otherwise.
    bool RomLoaded() const { return romLoaded_; };

    /// @brief Get the title of the currently loaded ROM.
    /// @return Title of ROM.
    std::string RomTitle() const { return romTitle_; }

    /// @brief Read an address on the cartridge. Includes ROM, SRAM, EEPROM, and Flash.
    /// @param addr Address to read.
    /// @param alignment BYTE, HALFWORD, or WORD.
    /// @return Value at specified address, number of cycles to read, and whether this read triggered open bus behavior.
    std::tuple<uint32_t, int, bool> ReadGamePak(uint32_t addr, AccessSize alignment);

    /// @brief Write to an address on the cartridge. Includes SRAM, EEPROM, and Flash.
    /// @param addr Address to write.
    /// @param value Value to write.
    /// @param alignment BYTE, HALFWORD, or WORD.
    /// @return Number of cycles taken to write.
    int WriteGamePak(uint32_t addr, uint32_t value, AccessSize alignment);

    /// @brief Check if this cartridge contains EEPROM and the address being accessed maps to it.
    /// @param addr Address being accessed.
    /// @return True if accessing EEPROM.
    bool EepromAccess(uint32_t addr) const;

    /// @brief Check if this cartridge contains SRAM and the address being accessed maps to it.
    /// @param addr Address being accessed.
    /// @return True if accessing SRAM.
    bool SramAccess(uint32_t addr) const;

    /// @brief Check if this cartridge contains Flash and the address being accessed maps to it.
    /// @param addr Address being accessed.
    /// @return True if accessing Flash.
    bool FlashAccess(uint32_t addr) const;

    /// @brief Set the EEPROM index to be read from on the next EEPROM read.
    /// @param index Index to read from.
    /// @param indexLength Number of bits (6 or 14) in index.
    /// @return Total number of cycles taken to write the index.
    int SetEepromIndex(size_t index, int indexLength);

    /// @brief Get the double word at previously set EEPROM index.
    /// @return Double word at EEPROM index and number of cycles taken to read it.
    std::pair<uint64_t, int> ReadFromEeprom();

    /// @brief Write a double word to EEPROM.
    /// @param index Index to write to.
    /// @param indexLength Number of bits (6 or 14) in index.
    /// @param value Double word to write to EEPROM.
    /// @return Total number of cycles taken to write to EEPROM.
    int WriteToEeprom(size_t index, int indexLength, uint64_t value);

private:
    std::tuple<uint32_t, int, bool> ReadROM(uint32_t addr, AccessSize alignment);

    /// @brief Read the ROM for a string indicating what type of backup media this cartridge contains.
    /// @return Backup type and if relevant, size of backup media in bytes.
    std::pair<BackupType, size_t> DetectBackupType();

    // ROM Info
    bool romLoaded_;
    std::string romTitle_;
    fs::path romPath_;

    // Memory
    std::vector<uint8_t> ROM_;

    // Backup Media
    BackupType backupType_;
    std::unique_ptr<EEPROM> eeprom_;
    std::unique_ptr<Flash> flash_;
    std::unique_ptr<SRAM> sram_;

    // Prefetch buffer and access timing info
    uint32_t lastAddrRead_;
};
}
