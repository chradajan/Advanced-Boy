#pragma once

#include <cstdint>
#include <CPU/CpuTypes.hpp>

namespace Audio
{
constexpr int SAMPLING_FREQUENCY_HZ = 32768;
constexpr int CPU_CYCLES_PER_SAMPLE = (CPU::CPU_FREQUENCY_HZ / SAMPLING_FREQUENCY_HZ);

// Maintain audio buffer of 22ms
constexpr size_t BUFFER_SIZE = ((SAMPLING_FREQUENCY_HZ * 22) / 1000) * 2;

constexpr int CPU_CYCLES_PER_GB_CYCLE = CPU::CPU_FREQUENCY_HZ / 1'048'576;
constexpr int CPU_CYCLES_PER_ENVELOPE_SWEEP = CPU::CPU_FREQUENCY_HZ / 64;
constexpr int CPU_CYCLES_PER_SOUND_LENGTH = CPU::CPU_FREQUENCY_HZ / 256;
constexpr int CPU_CYCLES_PER_FREQUENCY_SWEEP = CPU::CPU_FREQUENCY_HZ / 128;

constexpr int16_t MIN_OUTPUT_LEVEL = 0;
constexpr int16_t MAX_OUTPUT_LEVEL = 1023;

constexpr int8_t DUTY_CYCLE[4][8] =
{
    {1, 1, 1, 1, 1, 1, 1, 0},
    {1, 1, 1, 1, 1, 1, 0, 0},
    {1, 1, 1, 1, 0, 0, 0, 0},
    {1, 1, 0, 0, 0, 0, 0, 0}
};
}
