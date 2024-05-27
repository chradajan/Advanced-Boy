#pragma once

#include <array>
#include <cstdint>
#include <utility>
#include <Utilities/MemoryUtilities.hpp>

enum class InterruptType : uint16_t
{
    LCD_VBLANK              = 0x0001,
    LCD_HBLANK              = 0x0002,
    LCD_VCOUNTER_MATCH      = 0x0004,
    TIMER_0_OVERFLOW        = 0x0008,
    TIMER_1_OVERFLOW        = 0x0010,
    TIMER_2_OVERFLOW        = 0x0020,
    TIMER_3_OVERFLOW        = 0x0040,
    SERIAL_COMMUNICATION    = 0x0080,
    DMA0                    = 0x0100,
    DMA1                    = 0x0200,
    DMA2                    = 0x0400,
    DMA3                    = 0x0800,
    KEYPAD                  = 0x1000,
    GAME_PAK                = 0x2000
};

enum class WaitState
{
    ZERO,
    ONE,
    TWO,
    SRAM
};

/// @brief 
class SystemControl
{
public:
    /// @brief Initialize system control registers.
    SystemControl();

    /// @brief Zero out system control related registers.
    void Reset();

    /// @brief Check if the CPU should be halted.
    /// @return Whether the CPU is currently halted.
    bool Halted() const { return halted_; }

    /// @brief Read an interrupt, wait state, or power control register.
    /// @param addr Address of register to read.
    /// @param alignment Number of bytes to read.
    /// @return Value at specified register and whether this is an open bus read.
    std::pair<uint32_t, bool> ReadReg(uint32_t addr, AccessSize alignment);

    /// @brief Write an interrupt, waitstate, or power control register.
    /// @param addr Address of register to write.
    /// @param value Value to write to register.
    /// @param alignment Number of bytes to write.
    void WriteReg(uint32_t addr, uint32_t value, AccessSize alignment);

    /// @brief If an enabled interrupt is pending and the master interrupt control is enabled, schedule an IRQ.
    void CheckForInterrupt();

    /// @brief Set an interrupt flag in the IF register.
    /// @param interrupt Which interrupt type to request.
    void RequestInterrupt(InterruptType interrupt);

    /// @brief Get the number of wait states associated with a ROM or SRAM region.
    /// @param state Which wait state region to check.
    /// @param sequential Whether this access is sequential to the last one.
    /// @param alignment Number of bytes being read/written.
    /// @return Number of additional wait states for the current read/write operation.
    int WaitStates(WaitState state, bool sequential, AccessSize alignment) const;

    /// @brief Check if the GamePak prefetcher is enabled.
    /// @return Whether prefetcher is currently enabled.
    bool GamePakPrefetchEnabled() const { return waitcnt_.prefetchBuffer; }

private:
    /// @brief Handle reads to POSTFLG and HALTCNT registers.
    /// @param addr Address of register to read.
    /// @param alignment Number of bytes to read.
    /// @return Value at specified register and whether this is an open bus read.
    std::pair<uint32_t, bool> ReadPostFlgHaltcnt(uint32_t addr, AccessSize alignment);

    /// @brief Handle special behavior of writes to IE and IF registers.
    /// @param addr Address of register to write.
    /// @param value Value to write to register.
    /// @param alignment Number of bytes to write.
    void WriteInterruptWaitcnt(uint32_t addr, uint32_t value, AccessSize alignment);

    /// @brief Clear acknowledged interrupt flags in IF.
    /// @param acknowledgement Bitmap of interrupts to acknowledge.
    void AcknowledgeInterrupt(uint16_t acknowledgement);

    /// @brief Check if the CPU was halted by a write to HALTCNT. Halt CPU if necessary.
    /// @param addr Address of register to write.
    /// @param value Value to write to register.
    /// @param alignment Number of bytes to write.
    void CheckHaltWrite(uint32_t addr, uint32_t value, AccessSize alignment);

    // State
    bool halted_;

    // Registers
    std::array<uint8_t, 12> interruptAndWaitcntRegisters_;
    std::array<uint8_t, 4> postFlgAndHaltcntRegisters_;
    std::array<uint8_t, 4> undocumentedRegisters_;
    std::array<uint8_t, 4> internalMemoryControlRegisters_;

    uint16_t& ie_;
    uint16_t& if_;
    uint16_t& ime_;

    struct WAITCNT
    {
            uint16_t sramWaitCtrl : 2;
            uint16_t waitState0FirstAccess : 2;
            uint16_t waitState0SecondAccess : 1;
            uint16_t waitState1FirstAccess : 2;
            uint16_t waitState1SecondAccess : 1;
            uint16_t waitState2FirstAccess : 2;
            uint16_t waitState2SecondAccess : 1;
            uint16_t phiTerminalOutput : 2;
            uint16_t : 1;
            uint16_t prefetchBuffer : 1;
            uint16_t gamePakType : 1;
    };

    WAITCNT& waitcnt_;
};

extern SystemControl SystemController;
