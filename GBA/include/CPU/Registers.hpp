#pragma once

#include <CPU/CpuTypes.hpp>
#include <array>
#include <cstdint>
#include <string>

namespace CPU
{
constexpr uint8_t SP_INDEX = 13;
constexpr uint8_t LR_INDEX = 14;
constexpr uint8_t PC_INDEX = 15;

class Registers
{
public:
    /// @brief Initialize CPU registers.
    Registers();

    /// @brief Reset the ARM registers to their power-up state.
    void Reset();

    /// @brief If running without BIOS, initialize necessary registers to start execution from GamePak.
    void SkipBIOS();

    /// @brief Read a CPU register considering operating state and mode.
    /// @param cpu Pointer to CPU.
    /// @param index Index of register to read.
    /// @return Value of selected register.
    uint32_t ReadRegister(uint8_t index) const;

    /// @brief Read a CPU register from a specific operating mode.
    /// @param cpu Pointer to CPU.
    /// @param index Index of register to read.
    /// @param mode Operating mode to read registers of.
    /// @return Value of selected register.
    uint32_t ReadRegister(uint8_t index, OperatingMode mode) const;

    /// @brief Write a CPU register considering operating state and mode.
    /// @param cpu Pointer to CPU.
    /// @param index Index of register to write.
    /// @param value Value to set register to.
    void WriteRegister(uint8_t index, uint32_t value);

    /// @brief Write a CPU register from a specific operating mode.
    /// @param cpu Pointer to CPU.
    /// @param index Index of register to write.
    /// @param value Value to set register to.
    /// @param mode Operating mode to write register of.
    void WriteRegister(uint8_t index, uint32_t value, OperatingMode mode);

    /// @brief Get current value of PC considering operating state and mode.
    /// @param cpu Pointer to CPU.
    /// @return Value of program counter.
    uint32_t GetPC() const { return systemAndUserRegisters_.R15_; }

    /// @brief Set the PC.
    /// @param addr New address to set PC to.
    void SetPC(uint32_t addr) { systemAndUserRegisters_.R15_ = addr; }

    /// @brief Advance the PC by either 2 or 4 depending on current operating state.
    void AdvancePC() { systemAndUserRegisters_.R15_ += (GetOperatingState() == OperatingState::ARM) ? 4 : 2; }

    /// @brief Get the current stack pointer.
    /// @return Value of stack pointer.
    uint32_t GetSP() const;

    /// @brief Get the current link register.
    /// @return Value of link register.
    uint32_t GetLR() const;

    /// @brief Check if CPU is running in ARM or THUMB mode.
    /// @return Current CPU operating state.
    OperatingState GetOperatingState() const { return OperatingState{cpsr_.T}; }

    /// @brief Set the CPU operating state.
    /// @param state New operating state.
    void SetOperatingState(OperatingState state) { cpsr_.T = static_cast<uint32_t>(state); }

    /// @brief Check current mode of operation.
    /// @return Current CPU operating mode.
    OperatingMode GetOperatingMode() const { return OperatingMode{cpsr_.Mode}; }

    /// @brief Set the CPU operating mode.
    /// @param mode New operating mode.
    void SetOperatingMode(OperatingMode mode) { cpsr_.Mode = static_cast<uint32_t>(mode); }

    /// @brief Check if Negative/Less Than flag is set.
    /// @return Current state of N flag.
    bool IsNegative() const { return cpsr_.N; }

    /// @brief Update the Negative/Less Than flag.
    /// @param state New value to set N flag to.
    void SetNegative(bool state) { cpsr_.N = state; }

    /// @brief Check if Zero flag is set.
    /// @return Current state of Z flag.
    bool IsZero() const { return cpsr_.Z; }

    /// @brief Update the Zero flag.
    /// @param state New value to set Z flag to.
    void SetZero(bool state) { cpsr_.Z = state; }

    /// @brief Check if Carry/Borrow/Extend flag is set.
    /// @return Current state of C flag.
    bool IsCarry() const { return cpsr_.C; }

    /// @brief Update the Carry/Borrow/Extend flag.
    /// @param state New value to set C flag to.
    void SetCarry(bool state) { cpsr_.C = state; }

    /// @brief Check if Overflow flag is set.
    /// @return Current state of V flag.
    bool IsOverflow() const { return cpsr_.V; }

    /// @brief Update the Overflow flag.
    /// @param state New value to set V flag to.
    void SetOverflow(bool state) { cpsr_.V = state; }

    /// @brief Get the current value of CPSR.
    /// @return Current CPSR.
    uint32_t GetCPSR() const { return cpsr_.Register; }

    /// @brief Set the entire CPSR register to a new value.
    /// @param cpsr New value for CPSR register.
    void SetCPSR(uint32_t cpsr) { cpsr_.Register = cpsr; }

    /// @brief Get the current operating mode's SPSR value.
    /// @return Current SPSR.
    uint32_t GetSPSR() const;

    /// @brief Set the entire SPSR register for the current operating mode to a new value.
    /// @param spsr New value for SPSR register.
    void SetSPSR(uint32_t spsr);

    /// @brief Load the current operating mode's SPSR register into CPSR.
    void LoadSPSR();

    /// @brief Check if IRQ interrupts are disabled.
    /// @return Whether IRQ interrupts are currently disabled.
    bool IsIrqDisabled() const { return cpsr_.I; }

    /// @brief Set the IRQ disabled flag.
    /// @param state New value to set I flag to. true = disabled, false = enabled.
    void SetIrqDisabled(bool state) { cpsr_.I = state; }

    /// @brief Check if FIQ interrupts are disabled.
    /// @return Whether FIQ interrupts are currently disabled.
    bool IsFiqDisabled() const { return cpsr_.F; }

    /// @brief Set the FIQ disabled flag.
    /// @param state New value to set F flag to. true = disabled, false = enabled.
    void SetFiqDisabled(bool state) { cpsr_.F = state; }

    /// @brief Convert CPU registers to a human readable format.
    /// @return String representing current register state.
    void SetRegistersString(std::string& regString) const;

private:
    // CPSR layout
    union CPSR
    {
        struct
        {
            uint32_t Mode : 5;
            uint32_t T : 1;
            uint32_t F : 1;
            uint32_t I : 1;
            uint32_t : 20;
            uint32_t V : 1;
            uint32_t C : 1;
            uint32_t Z : 1;
            uint32_t N : 1;
        };

        uint32_t Register;
    };

    // Register data
    CPSR cpsr_;

    struct
    {
        uint32_t R0_;
        uint32_t R1_;
        uint32_t R2_;
        uint32_t R3_;
        uint32_t R4_;
        uint32_t R5_;
        uint32_t R6_;
        uint32_t R7_;
        uint32_t R8_;
        uint32_t R9_;
        uint32_t R10_;
        uint32_t R11_;
        uint32_t R12_;
        uint32_t R13_;
        uint32_t R14_;
        uint32_t R15_;
    } systemAndUserRegisters_;

    struct
    {
        uint32_t R8_;
        uint32_t R9_;
        uint32_t R10_;
        uint32_t R11_;
        uint32_t R12_;
        uint32_t R13_;
        uint32_t R14_;
        CPSR spsr_;
    } fiqRegisters_;

    struct
    {
        uint32_t R13_;
        uint32_t R14_;
        CPSR spsr_;
    } supervisorRegisters_;

    struct
    {
        uint32_t R13_;
        uint32_t R14_;
        CPSR spsr_;
    } abortRegisters_;

    struct
    {
        uint32_t R13_;
        uint32_t R14_;
        CPSR spsr_;
    } irqRegisters_;

    struct
    {
        uint32_t R13_;
        uint32_t R14_;
        CPSR spsr_;
    } undefinedRegisters_;

    // Register lookups
    std::array<uint32_t*, 16> systemAndUserRegistersLUT_ = {
        &systemAndUserRegisters_.R0_,
        &systemAndUserRegisters_.R1_,
        &systemAndUserRegisters_.R2_,
        &systemAndUserRegisters_.R3_,
        &systemAndUserRegisters_.R4_,
        &systemAndUserRegisters_.R5_,
        &systemAndUserRegisters_.R6_,
        &systemAndUserRegisters_.R7_,
        &systemAndUserRegisters_.R8_,
        &systemAndUserRegisters_.R9_,
        &systemAndUserRegisters_.R10_,
        &systemAndUserRegisters_.R11_,
        &systemAndUserRegisters_.R12_,
        &systemAndUserRegisters_.R13_,
        &systemAndUserRegisters_.R14_,
        &systemAndUserRegisters_.R15_
    };

    std::array<uint32_t*, 16> fiqRegistersLUT_ = {
        &systemAndUserRegisters_.R0_,
        &systemAndUserRegisters_.R1_,
        &systemAndUserRegisters_.R2_,
        &systemAndUserRegisters_.R3_,
        &systemAndUserRegisters_.R4_,
        &systemAndUserRegisters_.R5_,
        &systemAndUserRegisters_.R6_,
        &systemAndUserRegisters_.R7_,
        &fiqRegisters_.R8_,
        &fiqRegisters_.R9_,
        &fiqRegisters_.R10_,
        &fiqRegisters_.R11_,
        &fiqRegisters_.R12_,
        &fiqRegisters_.R13_,
        &fiqRegisters_.R14_,
        &systemAndUserRegisters_.R15_
    };

    std::array<uint32_t*, 16> supervisorRegistersLUT_ = {
        &systemAndUserRegisters_.R0_,
        &systemAndUserRegisters_.R1_,
        &systemAndUserRegisters_.R2_,
        &systemAndUserRegisters_.R3_,
        &systemAndUserRegisters_.R4_,
        &systemAndUserRegisters_.R5_,
        &systemAndUserRegisters_.R6_,
        &systemAndUserRegisters_.R7_,
        &systemAndUserRegisters_.R8_,
        &systemAndUserRegisters_.R9_,
        &systemAndUserRegisters_.R10_,
        &systemAndUserRegisters_.R11_,
        &systemAndUserRegisters_.R12_,
        &supervisorRegisters_.R13_,
        &supervisorRegisters_.R14_,
        &systemAndUserRegisters_.R15_
    };

    std::array<uint32_t*, 16> abortRegistersLUT_ = {
        &systemAndUserRegisters_.R0_,
        &systemAndUserRegisters_.R1_,
        &systemAndUserRegisters_.R2_,
        &systemAndUserRegisters_.R3_,
        &systemAndUserRegisters_.R4_,
        &systemAndUserRegisters_.R5_,
        &systemAndUserRegisters_.R6_,
        &systemAndUserRegisters_.R7_,
        &systemAndUserRegisters_.R8_,
        &systemAndUserRegisters_.R9_,
        &systemAndUserRegisters_.R10_,
        &systemAndUserRegisters_.R11_,
        &systemAndUserRegisters_.R12_,
        &abortRegisters_.R13_,
        &abortRegisters_.R14_,
        &systemAndUserRegisters_.R15_
    };

    std::array<uint32_t*, 16> irqRegistersLUT_ = {
        &systemAndUserRegisters_.R0_,
        &systemAndUserRegisters_.R1_,
        &systemAndUserRegisters_.R2_,
        &systemAndUserRegisters_.R3_,
        &systemAndUserRegisters_.R4_,
        &systemAndUserRegisters_.R5_,
        &systemAndUserRegisters_.R6_,
        &systemAndUserRegisters_.R7_,
        &systemAndUserRegisters_.R8_,
        &systemAndUserRegisters_.R9_,
        &systemAndUserRegisters_.R10_,
        &systemAndUserRegisters_.R11_,
        &systemAndUserRegisters_.R12_,
        &irqRegisters_.R13_,
        &irqRegisters_.R14_,
        &systemAndUserRegisters_.R15_
    };

    std::array<uint32_t*, 16> undefinedRegistersLUT_ = {
        &systemAndUserRegisters_.R0_,
        &systemAndUserRegisters_.R1_,
        &systemAndUserRegisters_.R2_,
        &systemAndUserRegisters_.R3_,
        &systemAndUserRegisters_.R4_,
        &systemAndUserRegisters_.R5_,
        &systemAndUserRegisters_.R6_,
        &systemAndUserRegisters_.R7_,
        &systemAndUserRegisters_.R8_,
        &systemAndUserRegisters_.R9_,
        &systemAndUserRegisters_.R10_,
        &systemAndUserRegisters_.R11_,
        &systemAndUserRegisters_.R12_,
        &undefinedRegisters_.R13_,
        &undefinedRegisters_.R14_,
        &systemAndUserRegisters_.R15_
    };
};
}
