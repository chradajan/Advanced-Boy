#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <utility>
#include <vector>
#include <Utilities/MemoryUtilities.hpp>

namespace fs = std::filesystem;

namespace Cartridge
{
constexpr size_t FLASH_BANK_SIZE = 64 * KiB;

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

class Flash
{
public:
    /// @brief Initialize Flash storage.
    /// @param savePath Path to save file. Load existing save if present.
    /// @param flashSizeInBytes Size of flash storage in bytes.
    Flash(fs::path savePath, size_t flashSizeInBytes);

    /// @brief Write save data to save file.
    ~Flash();

    /// @brief Reset Flash to power-up state.
    void Reset();

    /// @brief Read a byte in Flash storage or check its chip ID.
    /// @param addr Address in Flash to read.
    /// @param alignment Number of bytes to read.
    /// @return Value at specified address and number of cycles taken to read.
    std::pair<uint32_t, int> Read(uint32_t addr, AccessSize alignment);

    /// @brief Issue a Flash command or write a byte into Flash.
    /// @param addr Address to send command to, write to, or 4K sector to erase.
    /// @param value Command/value to write to address.
    /// @param alignment Number of bytes to write.
    /// @return Number of cycles taken to write/issue command.
    int Write(uint32_t addr, uint32_t value, AccessSize alignment);

private:
    fs::path const savePath_;
    FlashState state_;
    bool chipIdMode_;
    bool eraseMode_;
    size_t bank_;
    std::vector<std::array<uint8_t, FLASH_BANK_SIZE>> flash_;
};
}
