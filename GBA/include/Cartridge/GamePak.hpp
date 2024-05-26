#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include <System/MemoryMap.hpp>

namespace fs = std::filesystem;

namespace Cartridge
{
constexpr uint32_t EEPROM_ADDR_SMALL_CART_MIN = 0x0D00'0000;
constexpr uint32_t EEPROM_ADDR_LARGE_CART_MIN = 0x0DFF'FF00;
constexpr uint32_t EEPROM_ADDR_MAX = 0x0DFF'FFFF;

constexpr uint32_t SRAM_FLASH_ADDR_MIN = 0x0E00'0000;
constexpr uint32_t SRAM_FLASH_ADDR_MAX = 0x0FFF'FFFF;

constexpr uint32_t MAX_ROM_SIZE = 32 * MiB;
constexpr uint32_t ROM_ADDR_MAX = 0x0DFF'FFFF;

enum class BackupType : uint8_t
{
    None = 0,
    SRAM,
    EEPROM,
    FLASH
};

enum class FlashCommand : uint8_t
{
    START_CMD_SEQ = 0xAA,
    AWAIT_CMD = 0x55,

    ENTER_CHIP_ID_MODE = 0x90,
    EXIT_CHIP_ID_MODE = 0xF0,

    PREPARE_TO_RCV_ERASE_CMD = 0x80,
    ERASE_ENTIRE_CHIP = 0x10,
    ERASE_4K_SECTOR = 0x30,

    PREPARE_TO_WRITE_BYTE = 0xA0,

    SET_MEMORY_BANK = 0xB0,

    TERMINATE_WRITE_ERASE_CMD = 0xF0
};

enum class FlashState
{
    READY,
    CMD_SEQ_STARTED,
    AWAITING_CMD,

    ERASE_SEQ_READY,
    ERASE_SEQ_STARTED,
    AWAITING_ERASE_CMD,

    PREPARE_TO_WRITE,

    AWAITING_MEMORY_BANK
};

class GamePak
{
public:
    /// @brief Initialize a Game Pak.
    /// @param romPath Path to GBA ROM file.
    GamePak(fs::path romPath);

    /// @brief Destructor that handles creating save file upon unloading a Game Pak.
    ~GamePak();

    GamePak() = delete;
    GamePak(GamePak const&) = delete;
    GamePak(GamePak&&) = delete;
    GamePak& operator=(GamePak const&) = delete;
    GamePak& operator=(GamePak&&) = delete;

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

    /// @brief Get value of WAITCNT register.
    /// @return Value of WAITCNT.
    uint32_t ReadWAITCNT(uint32_t addr, AccessSize alignment);

    /// @brief Set value of WAITCNT register.
    /// @param value Value to set WAITCNT to.
    void WriteWAITCNT(uint32_t addr, uint32_t value, AccessSize alignment);

    /// @brief Check how many cycles it will take to access an address on the cartridge.
    /// @param addr Address being accessed.
    /// @param sequential Whether the address is sequential to the last accessed address.
    /// @param alignment BYTE, HALFWORD, or WORD.
    /// @return Number of cycles (1 + wait states) to access the specified address.
    int AccessTime(uint32_t addr, bool sequential, AccessSize alignment) const;

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
    void SetEepromIndex(size_t index, int indexLength);

    /// @brief Get the double word at previously set EEPROM index.
    /// @return Double word at EEPROM index.
    uint64_t GetEepromDoubleWord();

    /// @brief Write a double word to EEPROM.
    /// @param index Index to write to.
    /// @param indexLength Number of bits (6 or 14) in index.
    /// @param doubleWord Double word to write to EEPROM.
    void WriteToEeprom(size_t index, int indexLength, uint64_t doubleWord);

private:
    /// @brief Read an address in GamePak SRAM.
    /// @param addr Address to read.
    /// @param alignment BYTE, HALFWORD, or WORD.
    /// @return Value at specified address.
    uint32_t ReadSRAM(uint32_t addr, AccessSize alignment);

    /// @brief Write to an address in GamePak SRAM.
    /// @param addr Address to write.
    /// @param value Value to write.
    /// @param alignment BYTE, HALFWORD, or WORD.
    void WriteSRAM(uint32_t addr, uint32_t value, AccessSize alignment);

    /// @brief Read from an address in GamePak flash.
    /// @param addr Address to read.
    /// @param alignment BYTE, HALFWORD, or WORD.
    /// @return Value at specified address.
    uint32_t ReadFlash(uint32_t addr, AccessSize alignment);

    /// @brief Write to GamePak flash and interpret flash commands.
    /// @param addr Address to write.
    /// @param value Value to write or flash command.
    /// @param alignment BYTE, HALFWORD, or WORD.
    void WriteFlash(uint32_t addr, uint32_t value, AccessSize alignment);

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
    std::vector<uint8_t> backupMedia_;
    size_t eepromIndex_;
    FlashState flashState_;
    bool chipIdMode_;
    bool eraseMode_;
    size_t flashBank_;

    // Prefetch buffer and access timing info
    uint32_t lastAddrRead_;

    // Registers
    union WAITCNT
    {
        WAITCNT(uint32_t word) : word_(word) {}

        struct
        {
            uint32_t sramWaitCtrl : 2;
            uint32_t waitState0FirstAccess : 2;
            uint32_t waitState0SecondAccess : 1;
            uint32_t waitState1FirstAccess : 2;
            uint32_t waitState1SecondAccess : 1;
            uint32_t waitState2FirstAccess : 2;
            uint32_t waitState2SecondAccess : 1;
            uint32_t phiTerminalOutput : 2;
            uint32_t : 1;
            uint32_t prefetchBuffer : 1;
            uint32_t gamePakType : 1;
            uint32_t : 16;
        } flags_;

        uint32_t word_;
    } waitStateControl_;
};
}
