#pragma once

#include <ARM7TDMI/ARM7TDMI.hpp>
#include <Cartridge/GamePak.hpp>
#include <Gamepad/GamepadManager.hpp>
#include <Graphics/PPU.hpp>
#include <System/MemoryMap.hpp>
#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <tuple>
#include <utility>

namespace fs = std::filesystem;

class GameBoyAdvance
{
public:
    /// @brief Initialize the Game Boy Advance.
    /// @param biosPath Path to GBA BIOS file.
    /// @param refreshScreenCallback Function callback to use when screen is ready to be refreshed.
    GameBoyAdvance(fs::path biosPath, std::function<void(int)> refreshScreenCallback);

    GameBoyAdvance() = delete;
    GameBoyAdvance(GameBoyAdvance const&) = delete;
    GameBoyAdvance(GameBoyAdvance&&) = delete;
    GameBoyAdvance& operator=(GameBoyAdvance const&) = delete;
    GameBoyAdvance& operator=(GameBoyAdvance&&) = delete;
    ~GameBoyAdvance() = default;

    /// @brief Load a Game Pak into memory.
    /// @param romPath Path to ROM file.
    /// @return Whether the GamePak was valid and successfully loaded.
    bool LoadGamePak(fs::path romPath);

    /// @brief Update the KEYINPUT register based on current buttons being pressed.
    /// @param gamepad Current gamepad status.
    void UpdateGamepad(Gamepad gamepad);

    /// @brief Access the raw frame buffer data.
    /// @return Raw pointer to frame buffer.
    uint8_t* GetRawFrameBuffer() { return ppu_.GetRawFrameBuffer(); }

    /// @brief Run GBA indefinitely.
    void Run();

private:
    /// @brief Set all internal memory to 0.
    void ZeroMemory();

    /// @brief Read from memory.
    /// @param addr Address to read. Address is forcibly aligned to word/halfword boundary.
    /// @param alignment BYTE, HALFWORD, or WORD.
    /// @return Value from specified address and number of cycles taken to read.
    std::pair<uint32_t, int> ReadMemory(uint32_t addr, AccessSize alignment);

    /// @brief Write to memory.
    /// @param addr Address to read. Address is forcibly aligned to word/halfword boundary.
    /// @param value Value to write to specified address.
    /// @param alignment BYTE, HALFWORD, or WORD.
    /// @return Number of cycles taken to write.
    int WriteMemory(uint32_t addr, uint32_t value, AccessSize alignment);

    // Area specific R/W handling
    std::pair<uint32_t, int> ReadBIOS(uint32_t addr, AccessSize alignment);
    int WriteBIOS(uint32_t addr, uint32_t value, AccessSize alignment);

    std::pair<uint32_t, int> ReadOnBoardWRAM(uint32_t addr, AccessSize alignment);
    int WriteOnBoardWRAM(uint32_t addr, uint32_t value, AccessSize alignment);

    std::pair<uint32_t, int> ReadOnChipWRAM(uint32_t addr, AccessSize alignment);
    int WriteOnChipWRAM(uint32_t addr, uint32_t value, AccessSize alignment);

    std::tuple<uint32_t, int, bool> ReadIoReg(uint32_t addr, AccessSize alignment);
    int WriteIoReg(uint32_t addr, uint32_t value, AccessSize alignment);

    std::pair<uint32_t, int> ReadPaletteRAM(uint32_t addr, AccessSize alignment);
    int WritePaletteRAM(uint32_t addr, uint32_t value, AccessSize alignment);

    std::pair<uint32_t, int> ReadVRAM(uint32_t addr, AccessSize alignment);
    int WriteVRAM(uint32_t addr, uint32_t value, AccessSize alignment);

    std::pair<uint32_t, int> ReadOAM(uint32_t addr, AccessSize alignment);
    int WriteOAM(uint32_t addr, uint32_t value, AccessSize alignment);

    std::pair<uint32_t, int> ReadOpenBus(uint32_t addr, AccessSize alignment);

    // Components
    CPU::ARM7TDMI cpu_;
    GamepadManager gamepad_;
    Graphics::PPU ppu_;
    std::unique_ptr<Cartridge::GamePak> gamePak_;

    // Memory
    std::array<uint8_t,  16 * KiB> BIOS_;           // 00000000-00003FFF    BIOS - System ROM
    std::array<uint8_t, 256 * KiB> onBoardWRAM_;    // 02000000-0203FFFF    WRAM - On-board Work RAM
    std::array<uint8_t,  32 * KiB> onChipWRAM_;     // 03000000-03007FFF    WRAM - On-chip Work RAM
    std::array<uint8_t,   1 * KiB> paletteRAM_;     // 05000000-050003FF    BG/OBJ Palette RAM
    std::array<uint8_t,  96 * KiB> VRAM_;           // 06000000-06017FFF    VRAM - Video RAM
    std::array<uint8_t,   1 * KiB> OAM_;            // 07000000-070003FF    OAM - OBJ Attributes

    std::array<uint8_t, 0x804> placeholderIoRegisters_;

    bool biosLoaded_;
    bool gamePakLoaded_;
    int ppuCatchupCycles_;
};
