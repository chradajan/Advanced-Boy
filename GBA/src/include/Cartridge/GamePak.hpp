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
enum class BackupType : uint8_t
{
    None = 0,
    SRAM,
    EEPROM
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

    /// @brief Read an address in GamePak ROM.
    /// @param addr Address (will get aligned) to read.
    /// @param alignment BYTE, HALFWORD, or WORD.
    /// @return Value at specified address, number of cycles to read, and whether this read triggered open bus behavior.
    std::tuple<uint32_t, int, bool> ReadROM(uint32_t addr, AccessSize alignment);

    /// @brief Read an address in GamePak SRAM.
    /// @param addr Address (will get aligned) to read.
    /// @param alignment BYTE, HALFWORD, or WORD.
    /// @return Value at specified address and number of cycles to read.
    std::pair<uint32_t, int> ReadSRAM(uint32_t addr, AccessSize alignment);

    /// @brief Write to an address in GamePak SRAM.
    /// @param addr Address (will get aligned) to write.
    /// @param value Value to write.
    /// @param alignment BYTE, HALFWORD, or WORD.
    /// @return Number of cycles to write.
    int WriteSRAM(uint32_t addr, uint32_t value, AccessSize alignment);

    /// @brief Get value of WAITCNT register.
    /// @return Value of WAITCNT and number of cycles taken to read.
    std::pair<uint32_t, int> ReadWAITCNT(uint32_t addr, AccessSize alignment);

    /// @brief Set value of WAITCNT register.
    /// @param value Value to set WAITCNT to.
    /// @return Number of cycles to write.
    int WriteWAITCNT(uint32_t addr, uint32_t value, AccessSize alignment);

    /// @brief Get the number of cycles taken to non sequentially access an address in the GamePak.
    /// @return Number of non sequential cycles.
    int NonSequentialAccessTime() const;

    /// @brief Get the number of cycles take to sequentially access an address in the GamePak.
    /// @param addr Address being accessed.
    /// @return Number of sequential cycles.
    int SequentialAccessTime(uint32_t addr) const;

    /// @brief Check if EEPROM is being accessed via DMA.
    /// @param addr Address being accessed.
    /// @return True if accessing EEPROM.
    bool EepromAccess(uint32_t addr) const;

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
    /// @brief Determine which region of ROM is being accessed.
    /// @param addr ROM address being accessed.
    /// @return Which region is being accessed.
    int WaitStateRegion(uint32_t addr) const;

    /// @brief Determine the number of cycles to read ROM or SRAM.
    /// @param waitState Wait State 0, 1, 2 for ROM, or 3 for SRAM
    /// @return Number of cycles to read specified wait state.
    int WaitStateCycles(int waitState) { (void)waitState; return 1; };

    // ROM Info
    bool romLoaded_;
    std::string romTitle_;
    fs::path romPath_;

    // Memory
    std::vector<uint8_t> ROM_;

    // Backup Media
    BackupType backupType_;
    std::array<uint8_t, 64 * KiB> SRAM_;
    std::vector<uint64_t> EEPROM_;
    size_t eepromIndex_;

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
