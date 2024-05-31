#pragma once

#include <cstdint>
#include <CPU/CpuTypes.hpp>

namespace Audio
{
constexpr size_t SAMPLE_BUFFER_SIZE = 2048;
constexpr int SAMPLING_FREQUENCY_HZ = 44100;
constexpr float SAMPLING_PERIOD = 1.0 / SAMPLING_FREQUENCY_HZ;
constexpr int CPU_CYCLES_PER_SAMPLE = (CPU::CPU_FREQUENCY_HZ / SAMPLING_FREQUENCY_HZ);

constexpr int CPU_CYCLES_PER_GB_CYCLE = CPU::CPU_FREQUENCY_HZ / 1'048'576;
constexpr int CPU_CYCLES_PER_ENVELOPE_SWEEP = CPU::CPU_FREQUENCY_HZ / 64;
constexpr int CPU_CYCLES_PER_SOUND_LENGTH = CPU::CPU_FREQUENCY_HZ / 256;
constexpr int CPU_CYCLES_PER_FREQUENCY_SWEEP = CPU::CPU_FREQUENCY_HZ / 128;

constexpr int16_t MIN_OUTPUT_LEVEL = 0;
constexpr int16_t MAX_OUTPUT_LEVEL = 1023;

constexpr int8_t DUTY_CYCLE[4][8] =
{
    {1, 1, 1, 1, 1, 1, 1, -1},
    {1, 1, 1, 1, 1, 1, -1, -1},
    {1, 1, 1, 1, -1, -1, -1, -1},
    {1, 1, -1, -1, -1, -1, -1, -1}
};
}
