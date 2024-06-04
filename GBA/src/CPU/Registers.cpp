#include <CPU/Registers.hpp>
#include <cstdint>
#include <format>
#include <sstream>
#include <string>
#include <CPU/CpuTypes.hpp>
#include <System/SystemControl.hpp>

namespace CPU
{
Registers::Registers()
{
}

void Registers::Reset()
{
    cpsr_.Register = 0;
    SetOperatingMode(OperatingMode::Supervisor);
    SetOperatingState(OperatingState::ARM);
    SetIrqDisabled(true);
    SetFiqDisabled(true);

    systemAndUserRegisters_ = {};
    fiqRegisters_ = {};
    supervisorRegisters_ = {};
    abortRegisters_ = {};
    irqRegisters_ = {};
    undefinedRegisters_ = {};
}

void Registers::SkipBIOS()
{
    SetOperatingMode(OperatingMode::System);
    SetPC(0x0800'0000);
    WriteRegister(SP_INDEX, 0x0300'7F00, OperatingMode::System);
    WriteRegister(SP_INDEX, 0x0300'7FA0, OperatingMode::IRQ);
    WriteRegister(SP_INDEX, 0x0300'7FE0, OperatingMode::Supervisor);
}

uint32_t Registers::ReadRegister(uint8_t index) const
{
    auto mode = GetOperatingMode();
    return ReadRegister(index, mode);
}

uint32_t Registers::ReadRegister(uint8_t index, OperatingMode mode) const
{
    switch (mode)
    {
        case OperatingMode::User:
        case OperatingMode::System:
            return *systemAndUserRegistersLUT_.at(index);
        case OperatingMode::FIQ:
            return *fiqRegistersLUT_.at(index);
        case OperatingMode::Supervisor:
            return *supervisorRegistersLUT_.at(index);
        case OperatingMode::Abort:
            return *abortRegistersLUT_.at(index);
        case OperatingMode::IRQ:
            return *irqRegistersLUT_.at(index);
        case OperatingMode::Undefined:
            return *undefinedRegistersLUT_.at(index);
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
    if (index == PC_INDEX)
    {
        // Force align PC to either word or halfword depending on operating state.
        value &= (GetOperatingState() == OperatingState::ARM) ? 0xFFFF'FFFC : 0xFFFF'FFFE;
    }

    switch (mode)
    {
        case OperatingMode::User:
        case OperatingMode::System:
            *systemAndUserRegistersLUT_.at(index) = value;
            break;
        case OperatingMode::FIQ:
            *fiqRegistersLUT_.at(index) = value;
            break;
        case OperatingMode::Supervisor:
            *supervisorRegistersLUT_.at(index) = value;
            break;
        case OperatingMode::Abort:
            *abortRegistersLUT_.at(index) = value;
            break;
        case OperatingMode::IRQ:
            *irqRegistersLUT_.at(index) = value;
            break;
        case OperatingMode::Undefined:
            *undefinedRegistersLUT_.at(index) = value;
            break;
    }
}

uint32_t Registers::GetSP() const
{
    switch (GetOperatingMode())
    {
        case OperatingMode::User:
        case OperatingMode::System:
            return *systemAndUserRegistersLUT_.at(13);
        case OperatingMode::FIQ:
            return *fiqRegistersLUT_.at(13);
        case OperatingMode::Supervisor:
            return *supervisorRegistersLUT_.at(13);
        case OperatingMode::Abort:
            return *abortRegistersLUT_.at(13);
        case OperatingMode::IRQ:
            return *irqRegistersLUT_.at(13);
        case OperatingMode::Undefined:
            return *undefinedRegistersLUT_.at(13);
        default:
            return 0;
    }
}

uint32_t Registers::GetLR() const
{
    switch (GetOperatingMode())
    {
        case OperatingMode::User:
        case OperatingMode::System:
            return *systemAndUserRegistersLUT_.at(14);
        case OperatingMode::FIQ:
            return *fiqRegistersLUT_.at(14);
        case OperatingMode::Supervisor:
            return *supervisorRegistersLUT_.at(14);
        case OperatingMode::Abort:
            return *abortRegistersLUT_.at(14);
        case OperatingMode::IRQ:
            return *irqRegistersLUT_.at(14);
        case OperatingMode::Undefined:
            return *undefinedRegistersLUT_.at(14);
        default:
            return 0;
    }
}

void Registers::SetSPSR(uint32_t spsr)
{
    auto mode = GetOperatingMode();

    switch (mode)
    {
        case OperatingMode::FIQ:
            fiqRegisters_.spsr_.Register = spsr;
            break;
        case OperatingMode::Supervisor:
            supervisorRegisters_.spsr_.Register = spsr;
            break;
        case OperatingMode::Abort:
            abortRegisters_.spsr_.Register = spsr;
            break;
        case OperatingMode::IRQ:
            irqRegisters_.spsr_.Register = spsr;
            break;
        case OperatingMode::Undefined:
            undefinedRegisters_.spsr_.Register = spsr;
            break;
        default:
            break;
    }
}

uint32_t Registers::GetSPSR() const
{
    auto mode = GetOperatingMode();

    switch (mode)
    {
        case OperatingMode::FIQ:
            return fiqRegisters_.spsr_.Register;
        case OperatingMode::Supervisor:
            return supervisorRegisters_.spsr_.Register;
        case OperatingMode::Abort:
            return abortRegisters_.spsr_.Register;
        case OperatingMode::IRQ:
            return irqRegisters_.spsr_.Register;
        case OperatingMode::Undefined:
            return undefinedRegisters_.spsr_.Register;
        default:
            return cpsr_.Register;
    }
}

void Registers::LoadSPSR()
{
    auto mode = GetOperatingMode();

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

void Registers::SetRegistersString(std::string& regString) const
{
    std::stringstream regStream;
    bool const isThumbState = GetOperatingState() == OperatingState::THUMB;

    for (int i = 0; i < 16; ++i)
    {
        regStream << std::format("R{} {:08X}  ", i, ReadRegister(i));
    }

    regStream << "CPSR: " << (IsNegative() ? "N" : "-") << (IsZero() ? "Z" : "-") << (IsCarry() ? "C" : "-") << (IsOverflow() ? "V" : "-") << "  ";
    regStream << (IsIrqDisabled() ? "I" : "-") << (IsFiqDisabled() ? "F" : "-") << (isThumbState ? "T" : "-") << "  " << "Mode: ";
    uint32_t spsr = GetSPSR();

    switch (GetOperatingMode())
    {
        case OperatingMode::User:
            regStream << "User";
            break;
        case OperatingMode::FIQ:
            regStream << std::format("FIQ         SPSR: {:08X}", spsr);
            break;
        case OperatingMode::IRQ:
            regStream << std::format("IRQ         SPSR: {:08X}", spsr);
            break;
        case OperatingMode::Supervisor:
            regStream << std::format("Supervisor  SPSR: {:08X}", spsr);
            break;
        case OperatingMode::Abort:
            regStream << std::format("Abort       SPSR: {:08X}", spsr);
            break;
        case OperatingMode::System:
            regStream << "System";
            break;
        case OperatingMode::Undefined:
            regStream << std::format("Undefined   SPSR: {:08X}", spsr);
            break;
    }

    regString = regStream.str();
}
}  // namespace CPU
