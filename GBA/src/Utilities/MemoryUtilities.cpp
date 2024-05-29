#include <Utilities/MemoryUtilities.hpp>
#include <bit>
#include <cstdint>

uint32_t AlignAddress(uint32_t addr, AccessSize alignment)
{
    if (((addr & (static_cast<uint32_t>(alignment) - 1)) != 0))
    {
        addr &= ~(static_cast<uint32_t>(alignment) - 1);
    }

    return addr;
}

uint32_t ReadPointer(uint8_t* bytePtr, AccessSize alignment)
{
    uint32_t value = 0;

    switch (alignment)
    {
        case AccessSize::BYTE:
            value = *bytePtr;
            break;
        case AccessSize::HALFWORD:
            value = *reinterpret_cast<uint16_t*>(bytePtr);
            break;
        case AccessSize::WORD:
            value = *reinterpret_cast<uint32_t*>(bytePtr);
            break;
    }

    return value;
}

void WritePointer(uint8_t* bytePtr, uint32_t value, AccessSize alignment)
{
    switch (alignment)
    {
        case AccessSize::BYTE:
            *bytePtr = value;
            break;
        case AccessSize::HALFWORD:
            *reinterpret_cast<uint16_t*>(bytePtr) = value;
            break;
        case AccessSize::WORD:
            *reinterpret_cast<uint32_t*>(bytePtr) = value;
            break;
    }
}

int8_t SignExtend8(uint8_t input, size_t signBit)
{
    if (signBit > 7)
    {
        return input;
    }

    uint8_t signMask = 0x01 << signBit;

    if ((input & signMask) == signMask)
    {
        return static_cast<int8_t>(input | ~(signMask - 1));
    }

    return static_cast<int8_t>(input);
}

int16_t SignExtend16(uint16_t input, size_t signBit)
{
    if (signBit > 15)
    {
        return input;
    }

    uint16_t signMask = 0x01 << signBit;

    if ((input & signMask) == signMask)
    {
        return static_cast<int16_t>(input | ~(signMask - 1));
    }

    return static_cast<int16_t>(input);
}

int32_t SignExtend32(uint32_t input, size_t signBit)
{
    if (signBit > 31)
    {
        return input;
    }

    uint32_t signMask = 0x01 << signBit;

    if ((input & signMask) == signMask)
    {
        return static_cast<int32_t>(input | ~(signMask - 1));
    }

    return static_cast<int32_t>(input);
}

uint32_t Read8BitBus(uint8_t byte, AccessSize alignment)
{
    uint32_t value = byte;

    if (alignment == AccessSize::HALFWORD)
    {
        value *= 0x0101;
    }
    else if (alignment == AccessSize::WORD)
    {
        value *= 0x0101'0101;
    }

    return value;
}

uint8_t Write8BitBus(uint32_t addr, uint32_t value)
{
    return std::rotr(value, (addr & 0x03) * 8) & MAX_U8;
}
