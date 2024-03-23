#pragma once

#include <ARM7TDMI/CpuTypes.hpp>
#include <array>
#include <cstdint>
#include <string>

namespace CPU
{
class Registers
{
public:
    /// @brief Registers constructor. Initialize registers with values at system start-up.
    Registers();

    /// @brief Read a CPU register considering operating state and mode.
    /// @param cpu Pointer to CPU.
    /// @param index Index of register to read.
    /// @return Value of selected register.
    uint32_t ReadRegister(uint8_t index) const;

    /// @brief Write a CPU register considering operating state and mode.
    /// @param cpu Pointer to CPU.
    /// @param index Index of register to write.
    /// @param value Value to set register to.
    void WriteRegister(uint8_t index, uint32_t value);

    /// @brief Get current value of PC considering operating state and mode.
    /// @param cpu Pointer to CPU.
    /// @return Value of program counter.
    uint32_t GetPC() const { return systemAndUserRegisters_.r15_; }

    /// @brief Set the PC.
    /// @param addr New address to set PC to.
    void SetPC(uint32_t addr) { systemAndUserRegisters_.r15_ = addr; }

    /// @brief Advance the PC by either 2 or 4 depending on current operating state.
    void AdvancePC() { systemAndUserRegisters_.r15_ += (GetOperatingState() == OperatingState::ARM) ? 4 : 2; }

    /// @brief Get the current stack pointer. Only intended for use in THUMB mode.
    /// @return Value of stack pointer.
    uint32_t GetSP() const;

    /// @brief Get the current link register. Only intended for use in THUMB mode.
    /// @return Value of link register.
    uint32_t GetLR() const;

    /// @brief Check if CPU is running in ARM or THUMB mode.
    /// @return Current CPU operating state.
    OperatingState GetOperatingState() const { return OperatingState{cpsr_.flags_.t_}; }

    /// @brief Set the CPU operating state.
    /// @param state New operating state.
    void SetOperatingState(OperatingState state) { cpsr_.flags_.t_ = static_cast<uint32_t>(state); }

    /// @brief Check current mode of operation.
    /// @return Current CPU operating mode.
    OperatingMode GetOperatingMode() const { return OperatingMode{cpsr_.flags_.mode_}; }

    /// @brief Set the CPU operating mode.
    /// @param mode New operating mode.
    void SetOperatingMode(OperatingMode mode) { cpsr_.flags_.mode_ = static_cast<uint32_t>(mode); }

    /// @brief Check if Negative/Less Than flag is set.
    /// @return Current state of N flag.
    bool IsNegative() const { return cpsr_.flags_.n_; }

    /// @brief Update the Negative/Less Than flag.
    /// @param state New value to set N flag to.
    void SetNegative(bool state) { cpsr_.flags_.n_ = state; }

    /// @brief Check if Zero flag is set.
    /// @return Current state of Z flag.
    bool IsZero() const { return cpsr_.flags_.z_; }

    /// @brief Update the Zero flag.
    /// @param state New value to set Z flag to.
    void SetZero(bool state) { cpsr_.flags_.z_ = state; }

    /// @brief Check if Carry/Borrow/Extend flag is set.
    /// @return Current state of C flag.
    bool IsCarry() const { return cpsr_.flags_.c_; }

    /// @brief Update the Carry/Borrow/Extend flag.
    /// @param state New value to set C flag to.
    void SetCarry(bool state) { cpsr_.flags_.c_ = state; }

    /// @brief Check if Overflow flag is set.
    /// @return Current state of V flag.
    bool IsOverflow() const { return cpsr_.flags_.v_; }

    /// @brief Update the Overflow flag.
    /// @param state New value to set V flag to.
    void SetOverflow(bool state) { cpsr_.flags_.v_ = state; }

    /// @brief Check if IRQ interrupts are disabled.
    /// @return Whether IRQ interrupts are currently disabled.
    bool IsIrqDisabled() const { return cpsr_.flags_.i_; }

    /// @brief Set the IRQ disabled flag.
    /// @param state New value to set I flag to. true = disabled, false = enabled.
    void SetIrqDisabled(bool state) { cpsr_.flags_.i_ = state; }

    /// @brief Check if FIQ interrupts are disabled.
    /// @return Whether FIQ interrupts are currently disabled.
    bool IsFiqDisabled() const { return cpsr_.flags_.f_; }

    /// @brief Set the FIQ disabled flag.
    /// @param state New value to set F flag to. true = disabled, false = enabled.
    void SetFiqDisabled(bool state) { cpsr_.flags_.f_ = state; }

    /// @brief Convert CPU registers to a human readable format.
    /// @return String representing current register state.
    std::string GetRegistersString() const;

private:
    // CPSR layout
    union CPSR
    {
        struct
        {
            uint32_t mode_ : 5;
            uint32_t t_ : 1;
            uint32_t f_ : 1;
            uint32_t i_ : 1;
            uint32_t reserved_ : 20;
            uint32_t v_ : 1;
            uint32_t c_ : 1;
            uint32_t z_ : 1;
            uint32_t n_ : 1;
        } flags_;

        uint32_t word_;
    };

    // Register data
    CPSR cpsr_;

    struct
    {
        uint32_t r0_;
        uint32_t r1_;
        uint32_t r2_;
        uint32_t r3_;
        uint32_t r4_;
        uint32_t r5_;
        uint32_t r6_;
        uint32_t r7_;
        uint32_t r8_;
        uint32_t r9_;
        uint32_t r10_;
        uint32_t r11_;
        uint32_t r12_;
        uint32_t r13_;
        uint32_t r14_;
        uint32_t r15_;
    } systemAndUserRegisters_;

    struct
    {
        uint32_t r8_;
        uint32_t r9_;
        uint32_t r10_;
        uint32_t r11_;
        uint32_t r12_;
        uint32_t r13_;
        uint32_t r14_;
        CPSR spsr_;
    } fiqRegisters_;

    struct
    {
        uint32_t r13_;
        uint32_t r14_;
        CPSR spsr_;
    } supervisorRegisters_;

    struct
    {
        uint32_t r13_;
        uint32_t r14_;
        CPSR spsr_;
    } abortRegisters_;

    struct
    {
        uint32_t r13_;
        uint32_t r14_;
        CPSR spsr_;
    } irqRegisters_;

    struct
    {
        uint32_t r13_;
        uint32_t r14_;
        CPSR spsr_;
    } undefinedRegisters_;

    // Register lookups
    std::array<uint32_t*, 16> systemAndUserRegistersLUT_ = {
        &systemAndUserRegisters_.r0_,
        &systemAndUserRegisters_.r1_,
        &systemAndUserRegisters_.r2_,
        &systemAndUserRegisters_.r3_,
        &systemAndUserRegisters_.r4_,
        &systemAndUserRegisters_.r5_,
        &systemAndUserRegisters_.r6_,
        &systemAndUserRegisters_.r7_,
        &systemAndUserRegisters_.r8_,
        &systemAndUserRegisters_.r9_,
        &systemAndUserRegisters_.r10_,
        &systemAndUserRegisters_.r11_,
        &systemAndUserRegisters_.r12_,
        &systemAndUserRegisters_.r13_,
        &systemAndUserRegisters_.r14_,
        &systemAndUserRegisters_.r15_
    };

    std::array<uint32_t*, 16> fiqRegistersLUT_ = {
        &systemAndUserRegisters_.r0_,
        &systemAndUserRegisters_.r1_,
        &systemAndUserRegisters_.r2_,
        &systemAndUserRegisters_.r3_,
        &systemAndUserRegisters_.r4_,
        &systemAndUserRegisters_.r5_,
        &systemAndUserRegisters_.r6_,
        &systemAndUserRegisters_.r7_,
        &fiqRegisters_.r8_,
        &fiqRegisters_.r9_,
        &fiqRegisters_.r10_,
        &fiqRegisters_.r11_,
        &fiqRegisters_.r12_,
        &fiqRegisters_.r13_,
        &fiqRegisters_.r14_,
        &systemAndUserRegisters_.r15_
    };

    std::array<uint32_t*, 16> supervisorRegistersLUT_ = {
        &systemAndUserRegisters_.r0_,
        &systemAndUserRegisters_.r1_,
        &systemAndUserRegisters_.r2_,
        &systemAndUserRegisters_.r3_,
        &systemAndUserRegisters_.r4_,
        &systemAndUserRegisters_.r5_,
        &systemAndUserRegisters_.r6_,
        &systemAndUserRegisters_.r7_,
        &systemAndUserRegisters_.r8_,
        &systemAndUserRegisters_.r9_,
        &systemAndUserRegisters_.r10_,
        &systemAndUserRegisters_.r11_,
        &systemAndUserRegisters_.r12_,
        &supervisorRegisters_.r13_,
        &supervisorRegisters_.r14_,
        &systemAndUserRegisters_.r15_
    };

    std::array<uint32_t*, 16> abortRegistersLUT_ = {
        &systemAndUserRegisters_.r0_,
        &systemAndUserRegisters_.r1_,
        &systemAndUserRegisters_.r2_,
        &systemAndUserRegisters_.r3_,
        &systemAndUserRegisters_.r4_,
        &systemAndUserRegisters_.r5_,
        &systemAndUserRegisters_.r6_,
        &systemAndUserRegisters_.r7_,
        &systemAndUserRegisters_.r8_,
        &systemAndUserRegisters_.r9_,
        &systemAndUserRegisters_.r10_,
        &systemAndUserRegisters_.r11_,
        &systemAndUserRegisters_.r12_,
        &abortRegisters_.r13_,
        &abortRegisters_.r14_,
        &systemAndUserRegisters_.r15_
    };

    std::array<uint32_t*, 16> irqRegistersLUT_ = {
        &systemAndUserRegisters_.r0_,
        &systemAndUserRegisters_.r1_,
        &systemAndUserRegisters_.r2_,
        &systemAndUserRegisters_.r3_,
        &systemAndUserRegisters_.r4_,
        &systemAndUserRegisters_.r5_,
        &systemAndUserRegisters_.r6_,
        &systemAndUserRegisters_.r7_,
        &systemAndUserRegisters_.r8_,
        &systemAndUserRegisters_.r9_,
        &systemAndUserRegisters_.r10_,
        &systemAndUserRegisters_.r11_,
        &systemAndUserRegisters_.r12_,
        &irqRegisters_.r13_,
        &irqRegisters_.r14_,
        &systemAndUserRegisters_.r15_
    };

    std::array<uint32_t*, 16> undefinedRegistersLUT_ = {
        &systemAndUserRegisters_.r0_,
        &systemAndUserRegisters_.r1_,
        &systemAndUserRegisters_.r2_,
        &systemAndUserRegisters_.r3_,
        &systemAndUserRegisters_.r4_,
        &systemAndUserRegisters_.r5_,
        &systemAndUserRegisters_.r6_,
        &systemAndUserRegisters_.r7_,
        &systemAndUserRegisters_.r8_,
        &systemAndUserRegisters_.r9_,
        &systemAndUserRegisters_.r10_,
        &systemAndUserRegisters_.r11_,
        &systemAndUserRegisters_.r12_,
        &undefinedRegisters_.r13_,
        &undefinedRegisters_.r14_,
        &systemAndUserRegisters_.r15_
    };
};
}  // namespace CPU
