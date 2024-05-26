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
#include <Audio/SoundController.hpp>
#include <Cartridge/GamePak.hpp>
#include <Gamepad/GamepadManager.hpp>
#include <Graphics/PPU.hpp>
#include <System/DmaChannel.hpp>
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

    /// @brief Run the emulator until a specified number of audio samples are generated.
    /// @param samples Number of samples to generate.
    void FillAudioBuffer(int samples);

    /// @brief Empty the internal audio buffer into another buffer.
    /// @param buffer Buffer to load samples into.
    void DrainAudioBuffer(float* buffer);

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
    /// @brief Set all internal memory to 0.
    void ZeroMemory();

    /// @brief Load GBA BIOS into memory.
    /// @param biosPath Path to GBA BIOS.
    /// @return Whether valid BIOS was loaded.
    bool LoadBIOS(fs::path biosPath);

    /// @brief Callback to handle a CPU halt.
    void Halt(int) { halted_ = !halted_; }

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

    /// @brief Callback function to execute Dma Channel 0 transfer.
    void DMA0(int) { ExecuteDMA(0); }

    /// @brief Callback function to execute Dma Channel 1 transfer.
    void DMA1(int) { ExecuteDMA(1); }

    /// @brief Callback function to execute Dma Channel 2 transfer.
    void DMA2(int) { ExecuteDMA(2); }

    /// @brief Callback function to execute Dma Channel 3 transfer.
    void DMA3(int) { ExecuteDMA(3); }

    /// @brief Run the currently active DMA channel, and prepare to run the next highest priority channel if one is waiting.
    /// @param dmaChannelIndex DMA transfer channel to execute.
    void ExecuteDMA(int dmaChannelIndex);

    /// @brief Schedule a DMA transfer, partially run any channels that are being interrupted by a higher priority channel, and
    ///        update the scheduler to update the timing of any DMA events currently in the queue.
    /// @param enabledDmaChannels Array of which channels are enabled for a given transfer mode.
    void ScheduleDMA(std::array<bool, 4>& enabledDmaChannels);

    // Area specific R/W handling
    std::pair<uint32_t, int> ReadBIOS(uint32_t addr, AccessSize alignment);

    std::pair<uint32_t, int> ReadOnBoardWRAM(uint32_t addr, AccessSize alignment);
    int WriteOnBoardWRAM(uint32_t addr, uint32_t value, AccessSize alignment);

    std::pair<uint32_t, int> ReadOnChipWRAM(uint32_t addr, AccessSize alignment);
    int WriteOnChipWRAM(uint32_t addr, uint32_t value, AccessSize alignment);

    std::pair<uint32_t, bool> ReadIoReg(uint32_t addr, AccessSize alignment);
    void WriteIoReg(uint32_t addr, uint32_t value, AccessSize alignment);

    std::pair<uint32_t, int> ReadPaletteRAM(uint32_t addr, AccessSize alignment);
    int WritePaletteRAM(uint32_t addr, uint32_t value, AccessSize alignment);

    std::pair<uint32_t, int> ReadVRAM(uint32_t addr, AccessSize alignment);
    int WriteVRAM(uint32_t addr, uint32_t value, AccessSize alignment);

    std::pair<uint32_t, int> ReadOAM(uint32_t addr, AccessSize alignment);
    int WriteOAM(uint32_t addr, uint32_t value, AccessSize alignment);

    std::pair<uint32_t, bool> ReadDmaReg(uint32_t addr, AccessSize alignment);
    void WriteDmaReg(uint32_t addr, uint32_t value, AccessSize alignment);

    std::pair<uint32_t, int> ReadOpenBus(uint32_t addr, AccessSize alignment);

    // State
    bool biosLoaded_;
    bool gamePakLoaded_;
    bool halted_;

    // Components
    Audio::SoundController apu_;
    CPU::ARM7TDMI cpu_;
    GamepadManager gamepad_;
    Graphics::PPU ppu_;
    TimerManager timerManager_;
    std::unique_ptr<Cartridge::GamePak> gamePak_;

    // DMA channels
    std::array<DmaChannel, 4> dmaChannels_;
    std::array<bool, 4> dmaImmediately_;
    std::array<bool, 4> dmaOnVBlank_;
    std::array<bool, 4> dmaOnHBlank_;
    std::array<bool, 4> dmaOnReplenishA_;
    std::array<bool, 4> dmaOnReplenishB_;
    std::optional<int> activeDmaChannel_;

    // Memory
    std::array<uint8_t,  16 * KiB> BIOS_;           // 00000000-00003FFF    BIOS - System ROM
    std::array<uint8_t, 256 * KiB> onBoardWRAM_;    // 02000000-0203FFFF    WRAM - On-board Work RAM
    std::array<uint8_t,  32 * KiB> onChipWRAM_;     // 03000000-03007FFF    WRAM - On-chip Work RAM
    std::array<uint8_t,   1 * KiB> paletteRAM_;     // 05000000-050003FF    BG/OBJ Palette RAM
    std::array<uint8_t,  96 * KiB> VRAM_;           // 06000000-06017FFF    VRAM - Video RAM
    std::array<uint8_t,   1 * KiB> OAM_;            // 07000000-070003FF    OAM - OBJ Attributes

    std::array<uint8_t, 0x804> placeholderIoRegisters_;

    // Open bus
    uint32_t lastBiosFetch_;
    uint32_t lastReadValue_;

    // Friends
    friend class DmaChannel;
};
