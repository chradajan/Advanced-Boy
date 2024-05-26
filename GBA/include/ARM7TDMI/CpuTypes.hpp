#pragma once

#include <cstdint>
#include <limits>
#include <string>

namespace CPU
{
constexpr int CPU_FREQUENCY_HZ = 16'777'216;

class ARM7TDMI;

/// @brief Operating state of CPU (either ARM or THUMB instructions).
enum class OperatingState : uint32_t
{
    ARM = 0,  // Executing 32-bit ARM instructions
    THUMB = 1  // Executing 16-bit THUMB instructions
};

/// @brief Operating mode of CPU.
enum class OperatingMode : uint32_t
{
    User = 0b10000,  // The normal ARM program execution state
    FIQ = 0b10001,  // Designed to support a data transfer or channel process
    IRQ = 0b10010,  // Used for general-purpose interrupt handling
    Supervisor = 0b10011,  // Protected mode for the operating system
    Abort = 0b10111,  // Entered after a data or instruction prefetch abort
    Undefined = 0b11011,  // Entered when an undefined instruction is executed
    System = 0b11111  // A privileged user mode for the operating system
};

/// @brief Abstract base class of all ARM and THUMB instructions.
class Instruction
{
public:
    virtual ~Instruction() {}

    /// @brief Execute the instruction.
    /// @param cpu Reference to the ARM CPU.
    /// @return Number of cycles this instruction took to execute.
    virtual int Execute(ARM7TDMI& cpu) = 0;

    /// @brief Get the ARM/THUMB mnemonic of this instruction.
    /// @return ARM/THUMB mnemonic as a string.
    std::string GetMnemonic() const { return mnemonic_; }

protected:
    /// @brief Human readable form of this instruction.
    std::string mnemonic_;
};
}  // namespace CPU
