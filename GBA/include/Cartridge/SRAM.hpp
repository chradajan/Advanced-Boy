#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <utility>
#include <Utilities/MemoryUtilities.hpp>

namespace fs = std::filesystem;

namespace Cartridge
{
class SRAM
{
public:
    /// @brief Initialize SRAM storage.
    /// @param savePath Path to save file. Load existing save if present.
    SRAM(fs::path savePath);

    /// @brief Write save data to save file.
    ~SRAM();

    /// @brief Read a byte from SRAM.
    /// @param addr Address to read.
    /// @param alignment Number of bytes to read.
    /// @return Value at specified address in SRAM and number of cycles taken to read.
    std::pair<uint32_t, int> Read(uint32_t addr, AccessSize alignment);

    /// @brief Write to SRAM.
    /// @param addr Address to write.
    /// @param value Value to write to SRAM.
    /// @param alignment Number of bytes to write.
    /// @return Number of cycles taken to write.
    int Write(uint32_t addr, uint32_t value, AccessSize alignment);

private:
    fs::path const savePath_;
    std::array<uint8_t, 32 * KiB> sram_;
};
}
