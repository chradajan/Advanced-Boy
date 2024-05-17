#include <Gamepad/GamepadManager.hpp>
#include <array>
#include <cstdint>
#include <utility>
#include <System/InterruptManager.hpp>
#include <System/MemoryMap.hpp>
#include <Utilities/Utilities.hpp>

GamepadManager::GamepadManager() :
    gamepadRegisters_(),
    KEYINPUT_(*reinterpret_cast<Gamepad*>(&gamepadRegisters_.at(0))),
    KEYCNT_(*reinterpret_cast<Gamepad*>(&gamepadRegisters_.at(2)))
{
    KEYINPUT_ = Gamepad();
    KEYCNT_.halfword_ = 0;
}

void GamepadManager::UpdateGamepad(Gamepad gamepad)
{
    KEYINPUT_ = gamepad;
    CheckForGamepadIRQ();
}

std::pair<uint32_t, bool> GamepadManager::ReadReg(uint32_t addr, AccessSize alignment)
{
    size_t index = addr - KEYPAD_INPUT_IO_ADDR_MIN;
    uint8_t* bytePtr = &(gamepadRegisters_.at(index));
    uint32_t value = ReadPointer(bytePtr, alignment);
    return {value, false};
}

void GamepadManager::WriteReg(uint32_t addr, uint32_t value, AccessSize alignment)
{
    size_t index = addr - KEYPAD_INPUT_IO_ADDR_MIN;

    if (index <= 1)
    {
        if (alignment == AccessSize::WORD)
        {
            value >>= 16;
            KEYCNT_.halfword_ = value;
        }
        else
        {
            return;
        }
    }
    else
    {
        uint8_t* bytePtr = &(gamepadRegisters_.at(index));
        WritePointer(bytePtr, value, alignment);
    }

    CheckForGamepadIRQ();
}

void GamepadManager::CheckForGamepadIRQ()
{
    if (!KEYCNT_.buttons_.IRQ)
    {
        return;
    }

    bool gamepadIRQ = false;
    uint16_t mask = KEYCNT_.halfword_ & 0x03FF;

    if (KEYCNT_.buttons_.Cond)
    {
        // Logical AND
        gamepadIRQ = ((KEYINPUT_.halfword_ & mask) == mask);
    }
    else
    {
        // LOGICAL OR
        gamepadIRQ = ((KEYINPUT_.halfword_ & mask) != 0);
    }

    if (gamepadIRQ)
    {
        InterruptMgr.RequestInterrupt(InterruptType::KEYPAD);
    }
}
