#pragma once

#include <cstdint>
#include <limits>

enum class AccessSize : uint8_t
{
    BYTE = 1,
    HALFWORD = 2,
    WORD = 4
};

enum class ReadStatus
{
    VALID,
    OPEN_BUS,
    ZERO
};

// Memory size constants
constexpr uint32_t KiB = 1024;
constexpr uint32_t MiB = KiB * KiB;

// Useful 64-bit constants
constexpr uint64_t MSB_64 = 0x8000'0000'0000'0000;
constexpr uint64_t MAX_U64 = std::numeric_limits<uint64_t>::max();

// Useful 32-bit constants
constexpr uint32_t MSB_32 = 0b1000'0000'0000'0000'0000'0000'0000'0000;
constexpr uint32_t MAX_U32 = std::numeric_limits<uint32_t>::max();

// Useful 16-bit constants
constexpr uint16_t MSB_16 = 0b1000'0000'0000'0000;
constexpr uint16_t MAX_U16 = std::numeric_limits<uint16_t>::max();

// Useful 8-bit constants
constexpr uint8_t MSB_8 = 0b1000'0000;
constexpr uint8_t MAX_U8 = std::numeric_limits<uint8_t>::max();

// Generic Read/Write Functions

/// @brief Forcibly align unaligned address to down to nearest aligned address.
/// @param addr Address to align.
/// @param alignment Access size.
/// @return Aligned address.
uint32_t AlignAddress(uint32_t addr, AccessSize alignment);

/// @brief Read a byte, halfword, or word from an aligned pointer.
/// @param bytePtr Pointer to a byte on a properly aligned address.
/// @param alignment Access size.
/// @return Value at specified address.
uint32_t ReadPointer(uint8_t* bytePtr, AccessSize alignment);

/// @brief Write a byte, halfword, or word to an aligned pointer.
/// @param bytePtr Pointer to a byte on a properly aligned address.
/// @param alignment Access size.
/// @return Value to write to specified address.
void WritePointer(uint8_t* bytePtr, uint32_t value, AccessSize alignment);

/// @brief Sign extend to an 8 bit signed type.
/// @param input Unsigned value to sign extend.
/// @param signBit Which bit is the current sign bit.
/// @return 8 bit signed value.
int8_t SignExtend8(uint8_t input, size_t signBit);

/// @brief Sign extend to a 16 bit signed type.
/// @param input Unsigned value to sign extend.
/// @param signBit Which bit is the current sign bit.
/// @return 16 bit signed value.
int16_t SignExtend16(uint16_t input, size_t signBit);

/// @brief Sign extend to a 32 bit signed type.
/// @param input Unsigned value to sign extend.
/// @param signBit Which bit is the current sign bit.
/// @return 32 bit signed value.
int32_t SignExtend32(uint32_t input, size_t signBit);

/// @brief Rotate a byte read from an 8-bit bus without byte alignment.
/// @param byte Value at address being read.
/// @param alignment Alignment that was read with.
/// @return Rotated value.
uint32_t Read8BitBus(uint8_t byte, AccessSize alignment);

/// @brief Rotate a value written to an address on an 8-bit bus.
/// @param addr Address being written.
/// @param value Value to write to address.
/// @return Rotated value to actually write.
uint8_t Write8BitBus(uint32_t addr, uint32_t value);
