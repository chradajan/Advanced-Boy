#include <ARM7TDMI/ARM7TDMI.hpp>
#include <ARM7TDMI/ArmInstructions.hpp>
#include <ARM7TDMI/CpuTypes.hpp>
#include <ARM7TDMI/ThumbInstructions.hpp>
#include <cstdint>
#include <functional>

namespace CPU
{
ARM7TDMI::ARM7TDMI(std::function<uint32_t(uint32_t, uint8_t)> ReadMemory,
                   std::function<void(uint32_t, uint32_t, uint8_t)> WriteMemory) :
    ReadMemory(ReadMemory),
    WriteMemory(WriteMemory)
{
}
}
