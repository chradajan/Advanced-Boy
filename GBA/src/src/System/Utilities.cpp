#include <System/Utilities.hpp>
#include <System/MemoryMap.hpp>
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
    uint32_t value;

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
