#pragma once

#include <CPU/ARM7TDMI.hpp>
#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <Audio/APU.hpp>
#include <Cartridge/GamePak.hpp>
#include <DMA/DmaChannel.hpp>
#include <DMA/DmaManager.hpp>
#include <Gamepad/GamepadManager.hpp>
#include <Graphics/PPU.hpp>
#include <Timers/TimerManager.hpp>
#include <Utilities/MemoryUtilities.hpp>

namespace fs = std::filesystem;

class GameBoyAdvance
{
public:
    /// @brief Initialize the Game Boy Advance.
    /// @param biosPath Path to GBA BIOS file.
    GameBoyAdvance(fs::path biosPath);

    GameBoyAdvance() = delete;
    GameBoyAdvance(GameBoyAdvance const&) = delete;
    GameBoyAdvance(GameBoyAdvance&&) = delete;
    GameBoyAdvance& operator=(GameBoyAdvance const&) = delete;
    GameBoyAdvance& operator=(GameBoyAdvance&&) = delete;
    ~GameBoyAdvance() = default;

    /// @brief Reset the GBA and all its components to its power-up state.
    void Reset();

    /// @brief Run the emulator until a specified number of audio samples are generated.
    /// @param samples Number of samples to generate.
    void FillAudioBuffer(size_t samples);

    /// @brief Empty the internal audio buffer into another buffer.
    /// @param buffer Buffer to load samples into.
    void DrainAudioBuffer(float* buffer);

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

    /// @brief Get the number of times the PPU has hit VBlank since the last check.
    /// @return Number of times PPU has entered VBlank.
    int GetAndResetFrameCounter() { return ppu_.GetAndResetFrameCounter(); }

    /// @brief Get the title of the currently loaded ROM.
    /// @return Title of ROM.
    std::string RomTitle() const;

    /// @brief Dump log buffer to file.
    void DumpLogs() const;

private:
    /// @brief Top level function to read an address. Determine which memory region to route the read to. Force aligns address.
    /// @param addr Address to read from.
    /// @param alignment Number of bytes to read.
    /// @return Value at specified address and number of cycles taken to read.
    std::pair<uint32_t, int> ReadMemory(uint32_t addr, AccessSize alignment);

    /// @brief Top level function to write to an address. Determine which memory region to route the write to. Force aligns address.
    /// @param addr Address to write to.
    /// @param value Value to write to specified address.
    /// @param alignment Number of bytes to write.
    /// @return Number of cycles taken to write.
    int WriteMemory(uint32_t addr, uint32_t value, AccessSize alignment);

    /// @brief Load GBA BIOS into memory.
    /// @param biosPath Path to GBA BIOS.
    /// @return Whether valid BIOS was loaded.
    bool LoadBIOS(fs::path biosPath);

    /// @brief Callback function upon entering HBlank. Updates PPU and checks for HBlank DMAs.
    /// @param extraCycles Number of cycles that passed since this event was supposed to execute.
    void HBlank(int extraCycles);

    /// @brief Callback function upon entering VBlank. Updates PPU and checks for VBlank DMAs.
    /// @param extraCycles Number of cycles that passed since this event was supposed to execute.
    void VBlank(int extraCycles);

    /// @brief Callback function for a timer 0 overflow event.
    /// @param extraCycles Number of cycles that passed since this event was supposed to execute.
    void Timer0Overflow(int extraCycles) { TimerOverflow(0, extraCycles); }

    /// @brief Callback function for a timer 1 overflow event.
    /// @param extraCycles Number of cycles that passed since this event was supposed to execute.
    void Timer1Overflow(int extraCycles) { TimerOverflow(1, extraCycles); }

    /// @brief Handle timer overflow events potentially connected to DMA-Sound playback.
    /// @param timer Which timer overflowed (0 or 1).
    /// @param extraCycles Number of cycles that passed since the overflow happened.
    void TimerOverflow(int timer, int extraCycles);

    // Area specific R/W handling

    //                Bus   Read      Write     Cycles
    //  BIOS ROM      32    8/16/32   -         1/1/1
    std::pair<uint32_t, int> ReadBIOS(uint32_t addr, AccessSize alignment);

    //  Region        Bus   Read      Write     Cycles
    //  Work RAM 256K 16    8/16/32   8/16/32   3/3/6 **
    std::pair<uint32_t, int> ReadOnBoardWRAM(uint32_t addr, AccessSize alignment);
    int WriteOnBoardWRAM(uint32_t addr, uint32_t value, AccessSize alignment);

    //                Bus   Read      Write     Cycles
    //  Work RAM 32K  32    8/16/32   8/16/32   1/1/1
    std::pair<uint32_t, int> ReadOnChipWRAM(uint32_t addr, AccessSize alignment);
    int WriteOnChipWRAM(uint32_t addr, uint32_t value, AccessSize alignment);

    //               Bus   Read      Write     Cycles
    // I/O           32    8/16/32   8/16/32   1/1/1
    std::pair<uint32_t, int> ReadIoReg(uint32_t addr, AccessSize alignment);
    int WriteIoReg(uint32_t addr, uint32_t value, AccessSize alignment);

    std::pair<uint32_t, int> ReadOpenBus(uint32_t addr, AccessSize alignment);

    // State
    bool const biosLoaded_;
    bool gamePakLoaded_;

    // Components
    Audio::APU apu_;
    CPU::ARM7TDMI cpu_;
    DmaManager dmaMgr_;
    GamepadManager gamepad_;
    Graphics::PPU ppu_;
    TimerManager timerMgr_;
    std::unique_ptr<Cartridge::GamePak> gamePak_;

    // Memory
    std::array<uint8_t,  16 * KiB> BIOS_;
    std::array<uint8_t, 256 * KiB> onBoardWRAM_;
    std::array<uint8_t,  32 * KiB> onChipWRAM_;

    std::array<uint8_t, 0x804> placeholderIoRegisters_;

    // Open bus
    uint32_t lastBiosFetch_;
    uint32_t lastReadValue_;

    // DMA friends
    friend class DmaChannel;
    friend class DmaManager;
};
