#pragma once

#include <GamePad.hpp>
#include <System/MemoryMap.hpp>
#include <array>
#include <cstdint>
#include <tuple>

class GamepadManager
{
public:
    /// @brief Initialize the Gamepad registers (no buttons pressed and no interrupts enabled).
    GamepadManager();

    /// @brief Update the KEYINPUT register based on currently pressed buttons and check for Gamepad IRQ.
    /// @param gamepad 
    void UpdateGamepad(Gamepad gamepad);

    /// @brief Read KEYINPUT or KEYCNT registers.
    /// @param addr Address to read.
    /// @param alignment BYTE, HALFWORD, or WORD.
    /// @return Value at specified address, number of cycles taken to read, and whether this read triggered open bus behavior.
    std::tuple<uint32_t, int, bool> ReadGamepadReg(uint32_t addr, AccessSize alignment);

    /// @brief Write KEYINPUT or KEYCNT registers.
    /// @param addr Address to write.
    /// @param value Value to write.
    /// @param alignment BYTE, HALFWORD, or WORD.
    /// @return Number of cycles taken to write.
    int WriteGamepadReg(uint32_t addr, uint32_t value, AccessSize alignment);

private:
    /// @brief Check if Gamepad IRQ should be sent.
    void CheckForGamepadIRQ();

    // Registers
    std::array<uint8_t, 4> gamepadRegisters_;
    Gamepad& KEYINPUT_;
    Gamepad& KEYCNT_;
};
