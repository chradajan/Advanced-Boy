#pragma once

#include <cstdint>
#include <filesystem>
#include <utility>
#include <vector>
#include <Utilities/MemoryUtilities.hpp>

namespace fs = std::filesystem;

namespace Cartridge
{
class EEPROM
{
public:
    /// @brief Initialize EEPROM storage.
    /// @param savePath Path to save file. Load existing save if present.
    EEPROM(fs::path savePath);

    /// @brief Write save data to save file.
    ~EEPROM();

    /// @brief Reset EEPROM to power-up state.
    void Reset();

    /// @brief Read an address mapped to EEPROM.
    /// @param addr Address to read.
    /// @param alignment Number of bytes to read.
    /// @return Always returns 1 and the number of cycles taken to read.
    std::pair<uint32_t, int> Read(uint32_t addr, AccessSize alignment);

    /// @brief Write to an address mapped to EEPROM. Write does not do anything.
    /// @param addr Address to write.
    /// @param value Value to write to address.
    /// @param alignment Number of bytes to write.
    /// @return Number of cycles taken to write.
    int Write(uint32_t addr, uint32_t value, AccessSize alignment);

    /// @brief Set the index of the next dword to read from EEPROM.
    /// @param index Index to be read from.
    /// @param indexLength Number of bits in index.
    /// @return Number of cycles taken to set index.
    int SetIndex(size_t index, size_t indexLength);

    /// @brief Read a dword from EEPROM based on the previously set index.
    /// @return Dword at current EEPROM index and number of cycles taken to read it.
    std::pair<uint64_t, int> ReadDoubleWord();

    /// @brief Write a dword into EEPROM.
    /// @param index Index to write to.
    /// @param indexLength Number of bits in index.
    /// @param value Dword to write into EEPROM.
    /// @return Number of cycles taken to write to EEPROM.
    int WriteDoubleWord(size_t index, size_t indexLength, uint64_t value);

private:
    fs::path const savePath_;
    uint16_t currentIndex_;
    std::vector<uint64_t> eeprom_;
};
}
