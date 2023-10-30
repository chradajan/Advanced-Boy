#include <ARM7TDMI/Registers.hpp>
#include <ARM7TDMI/CpuTypes.hpp>
#include <cassert>
#include <cstdint>

namespace CPU
{
Registers::Registers()
{
}

uint32_t Registers::ReadRegister(uint8_t index) const
{
    assert((index < 8) || ((GetOperatingState() == OperatingState::ARM) && (index < 16)));

    switch (GetOperatingMode())
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
    assert((index < 8) || ((GetOperatingState() == OperatingState::ARM) && (index < 16)));

    switch (GetOperatingMode())
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
    assert(GetOperatingState() == OperatingState::THUMB);

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
    assert(GetOperatingState() == OperatingState::THUMB);

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
}  // namespace CPU
