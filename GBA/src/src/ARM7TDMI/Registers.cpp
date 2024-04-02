#include <ARM7TDMI/Registers.hpp>
#include <ARM7TDMI/CpuTypes.hpp>
#include <Config.hpp>
#include <cassert>
#include <cstdint>
#include <format>
#include <sstream>
#include <string>

namespace CPU
{
Registers::Registers()
{
    SetOperatingMode(OperatingMode::System);
    SetOperatingState(OperatingState::ARM);

    systemAndUserRegisters_ = {};
    fiqRegisters_ = {};
    supervisorRegisters_ = {};
    abortRegisters_ = {};
    irqRegisters_ = {};
    undefinedRegisters_ = {};

    // TODO
    // SKip the BIOS and set PC to beginning of Game Pak ROM. Also set r0, r13, and r14.
    SetPC(0x08000000);

    *systemAndUserRegistersLUT_[0] = 0x0000'0CA5;
    *systemAndUserRegistersLUT_[13] = 0x0300'7F00;
    *systemAndUserRegistersLUT_[14] = 0x0800'0000;
}

uint32_t Registers::ReadRegister(uint8_t index) const
{
    auto mode = GetOperatingMode();
    return ReadRegister(index, mode);
}

uint32_t Registers::ReadRegister(uint8_t index, OperatingMode mode) const
{
    if constexpr (Config::ASSERTS_ENABLED)
    {
        assert((index < 8) || ((GetOperatingState() == OperatingState::ARM) && (index < 16)));
    }

    switch (mode)
    {
        case OperatingMode::User:
        case OperatingMode::System:
            return *systemAndUserRegistersLUT_[index];
        case OperatingMode::FIQ:
            return *fiqRegistersLUT_[index];
        case OperatingMode::Supervisor:
            return *supervisorRegistersLUT_[index];
        case OperatingMode::Abort:
            return *abortRegistersLUT_[index];
        case OperatingMode::IRQ:
            return *irqRegistersLUT_[index];
        case OperatingMode::Undefined:
            return *undefinedRegistersLUT_[index];
        default:
            return 0;
    }
}

void Registers::WriteRegister(uint8_t index, uint32_t value)
{
    auto mode = GetOperatingMode();
    WriteRegister(index, value, mode);
}

void Registers::WriteRegister(uint8_t index, uint32_t value, OperatingMode mode)
{
    if constexpr (Config::ASSERTS_ENABLED)
    {
        assert((index < 8) || ((GetOperatingState() == OperatingState::ARM) && (index < 16)));
    }

    switch (mode)
    {
        case OperatingMode::User:
        case OperatingMode::System:
            *systemAndUserRegistersLUT_[index] = value;
            break;
        case OperatingMode::FIQ:
            *fiqRegistersLUT_[index] = value;
            break;
        case OperatingMode::Supervisor:
            *supervisorRegistersLUT_[index] = value;
            break;
        case OperatingMode::Abort:
            *abortRegistersLUT_[index] = value;
            break;
        case OperatingMode::IRQ:
            *irqRegistersLUT_[index] = value;
            break;
        case OperatingMode::Undefined:
            *undefinedRegistersLUT_[index] = value;
            break;
    }
}

uint32_t Registers::GetSP() const
{
    if constexpr (Config::ASSERTS_ENABLED)
    {
        assert(GetOperatingState() == OperatingState::THUMB);
    }

    switch (GetOperatingMode())
    {
        case OperatingMode::User:
        case OperatingMode::System:
            return *systemAndUserRegistersLUT_[13];
        case OperatingMode::FIQ:
            return *fiqRegistersLUT_[13];
        case OperatingMode::Supervisor:
            return *supervisorRegistersLUT_[13];
        case OperatingMode::Abort:
            return *abortRegistersLUT_[13];
        case OperatingMode::IRQ:
            return *irqRegistersLUT_[13];
        case OperatingMode::Undefined:
            return *undefinedRegistersLUT_[13];
        default:
            return 0;
    }
}

uint32_t Registers::GetLR() const
{
    if constexpr (Config::ASSERTS_ENABLED)
    {
        assert(GetOperatingState() == OperatingState::THUMB);
    }

    switch (GetOperatingMode())
    {
        case OperatingMode::User:
        case OperatingMode::System:
            return *systemAndUserRegistersLUT_[14];
        case OperatingMode::FIQ:
            return *fiqRegistersLUT_[14];
        case OperatingMode::Supervisor:
            return *supervisorRegistersLUT_[14];
        case OperatingMode::Abort:
            return *abortRegistersLUT_[14];
        case OperatingMode::IRQ:
            return *irqRegistersLUT_[14];
        case OperatingMode::Undefined:
            return *undefinedRegistersLUT_[14];
        default:
            return 0;
    }
}

void Registers::SaveCPSR()
{
    auto mode = GetOperatingMode();

    assert((mode != OperatingMode::User) || (mode != OperatingMode::System));

    switch (mode)
    {
        case OperatingMode::FIQ:
            fiqRegisters_.spsr_ = cpsr_;
            break;
        case OperatingMode::Supervisor:
            supervisorRegisters_.spsr_ = cpsr_;
            break;
        case OperatingMode::Abort:
            abortRegisters_.spsr_ = cpsr_;
            break;
        case OperatingMode::IRQ:
            irqRegisters_.spsr_ = cpsr_;
            break;
        case OperatingMode::Undefined:
            undefinedRegisters_.spsr_ = cpsr_;
            break;
        default:
            break;
    }
}

void Registers::LoadSPSR()
{
    auto mode = GetOperatingMode();

    assert((mode != OperatingMode::User) || (mode != OperatingMode::System));

    switch (mode)
    {
        case OperatingMode::FIQ:
            cpsr_ = fiqRegisters_.spsr_;
            break;
        case OperatingMode::Supervisor:
            cpsr_ = supervisorRegisters_.spsr_;
            break;
        case OperatingMode::Abort:
            cpsr_ = abortRegisters_.spsr_;
            break;
        case OperatingMode::IRQ:
            cpsr_ = irqRegisters_.spsr_;
            break;
        case OperatingMode::Undefined:
            cpsr_ = undefinedRegisters_.spsr_;
            break;
        default:
            break;
    }
}

std::string Registers::GetRegistersString() const
{
    std::stringstream regStream;
    bool const isArmState = GetOperatingState() == OperatingState::ARM;

    for (int i = 0; i < 15; ++i)
    {
        regStream << std::format("r{} {:08X}  ", i, ReadRegister(i));
    }

    regStream << "CPSR: " << (IsNegative() ? "N" : " ") << (IsZero() ? "Z" : " ") << (IsCarry() ? "C" : " ") << (IsOverflow() ? "V" : " ") << "  ";
    regStream << (IsIrqDisabled() ? "I" : " ") << (IsFiqDisabled() ? "F" : " ") << (isArmState ? "T" : " ") << "  " << "Mode: ";

    switch (GetOperatingMode())
    {
        case OperatingMode::User:
            regStream << "User";
            break;
        case OperatingMode::FIQ:
            regStream << "FIQ";
            break;
        case OperatingMode::IRQ:
            regStream << "IRQ";
            break;
        case OperatingMode::Supervisor:
            regStream << "Supervisor";
            break;
        case OperatingMode::Abort:
            regStream << "Abort";
            break;
        case OperatingMode::System:
            regStream << "System";
            break;
        case OperatingMode::Undefined:
            regStream << "Undefined";
            break;
    }

    return regStream.str();
}
}  // namespace CPU
