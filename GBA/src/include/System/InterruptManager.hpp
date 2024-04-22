#pragma once

#include <System/MemoryMap.hpp>
#include <array>
#include <cstdint>
#include <tuple>

constexpr uint32_t IE_ADDR  = 0x0400'0200;
constexpr uint32_t IF_ADDR  = 0x0400'0202;
constexpr uint32_t IME_ADDR = 0x0400'0208;
constexpr uint32_t POSTFLG_ADDR = 0x0400'0300;
constexpr uint32_t HALTCNT_ADDR = 0x0400'0301;
constexpr uint32_t MIRRORED_IO_ADDR = 0x0400'0800;

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
    DMA_0                   = 0x0100,
    DMA_1                   = 0x0200,
    DMA_2                   = 0x0400,
    DMA_3                   = 0x0800,
    KEYPAD                  = 0x1000,
    GAME_PAK                = 0x2000
};

/// @brief Manager for determining when to fire IRQ events. Also holds several undocumented power-down control registers.
class InterruptManager
{
public:
    /// @brief Initialize the Interrupt Manager.
    InterruptManager();

    /// @brief Request an interrupt.
    /// @param interrupt Interrupt type to set the corresponding IF flag for.
    void RequestInterrupt(InterruptType interrupt);

    /// @brief Read an I/O register managed by this.
    /// @param addr Address of memory mapped I/O register.
    /// @param alignment BYTE, HALFWORD, or WORD.
    /// @return Value of specified register, number of cycles taken to read, and whether this read triggered open bus behavior.
    std::tuple<uint32_t, int, bool> ReadIoReg(uint32_t addr, AccessSize alignment);

    /// @brief Write an I/O register managed by this.
    /// @param addr Address of memory mapped I/O register.
    /// @param value Value to write to register.
    /// @param alignment BYTE, HALFWORD, or WORD.
    /// @return Number of cycles taken to write.
    int WriteIoReg(uint32_t addr, uint32_t value, AccessSize alignment);

private:
    /// @brief Check if interrupts and enabled and if an interrupt type is both enabled and requested. If so, schedule an IRQ.
    void CheckForInterrupt();

    // Interrupt, wait state, and power down controls
    std::array<uint8_t, 0x604> intWtstPwdDownRegisters_;

    // Interrupt registers
    uint16_t& IE_;   // Interrupt Enable
    uint16_t& IF_;   // Interrupt Request / IRQ Acknowledge
    uint16_t& IME_;  // Interrupt Master Enable

    // Undocumented registers
    uint8_t& POSTFLG_;
    uint8_t& HALTCNT_;

    // Stop/Halt Status
    bool halted_;
};

extern InterruptManager InterruptMgr;
