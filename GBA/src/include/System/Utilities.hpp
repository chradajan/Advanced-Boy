#pragma once

#include <System/MemoryMap.hpp>
#include <cstdint>

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
