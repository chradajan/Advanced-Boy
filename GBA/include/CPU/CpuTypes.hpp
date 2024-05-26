#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <Utilities/Functor.hpp>
#include <Utilities/MemoryUtilities.hpp>

namespace CPU { class ARM7TDMI; }
class GameBoyAdvance;

namespace CPU
{
typedef MemberFunctor<std::pair<uint32_t, int> (GameBoyAdvance::*)(uint32_t, AccessSize)> MemReadCallback;
typedef MemberFunctor<int (GameBoyAdvance::*)(uint32_t, uint32_t, AccessSize)> MemWriteCallback;

constexpr int CPU_FREQUENCY_HZ = 16'777'216;

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
    virtual ~Instruction() = default;

    /// @brief Execute the instruction.
    /// @param cpu Reference to the ARM CPU.
    virtual void Execute(ARM7TDMI& cpu) = 0;
};
}
