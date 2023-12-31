#pragma once

#include <cstdint>
#include <limits>

namespace CPU
{
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
    System = 0b11011,  // A privileged user mode for the operating system
    Undefined = 0b11111  // Entered when an undefined instruction is executed
};

/// @brief Abstract base class of all ARM and THUMB instructions.
class Instruction
{
public:
    virtual ~Instruction() {}

    /// @brief Execute the instruction.
    /// @param cpu Pointer to the ARM CPU.
    virtual void Execute(ARM7TDMI& cpu) = 0;
};

// Useful 32-bit constants
constexpr uint32_t MSB_32 = 0b1000'0000'0000'0000'0000'0000'0000'0000;
constexpr uint32_t MAX_U32 = std::numeric_limits<uint32_t>::max();

// Useful 16-bit constants
constexpr uint16_t MSB_16 = 0b1000'0000'0000'0000;
constexpr uint16_t MAX_U16 = std::numeric_limits<uint16_t>::max();

// Useful 8-bit constants
constexpr uint8_t MSB_8 = 0b1000'0000;
constexpr uint8_t MAX_U8 = std::numeric_limits<uint8_t>::max();
}  // namespace CPU
