#include <System/SystemControl.hpp>
#include <array>
#include <cstdint>
#include <System/EventScheduler.hpp>
#include <System/MemoryMap.hpp>

SystemControl SystemController;

static constexpr int NonSequentialWaitStates[4] = {4, 3, 2, 8};
static constexpr int SequentialWaitStates[3][2] = { {2, 1}, {4, 1}, {8, 1} };

SystemControl::SystemControl() :
    interruptAndWaitcntRegisters_(),
    postFlgAndHaltcntRegisters_(),
    ie_(*reinterpret_cast<uint16_t*>(&interruptAndWaitcntRegisters_[0])),
    if_(*reinterpret_cast<uint16_t*>(&interruptAndWaitcntRegisters_[2])),
    ime_(*reinterpret_cast<uint16_t*>(&interruptAndWaitcntRegisters_[8])),
    waitcnt_(*reinterpret_cast<WAITCNT*>(&interruptAndWaitcntRegisters_[4]))
{
}

void SystemControl::Reset()
{
    halted_ = false;
    interruptAndWaitcntRegisters_.fill(0);
    postFlgAndHaltcntRegisters_.fill(0);
    undocumentedRegisters_.fill(0);
    internalMemoryControlRegisters_.fill(0);
}

std::pair<uint32_t, bool> SystemControl::ReadReg(uint32_t addr, AccessSize alignment)
{
    uint32_t value = 0;
    uint8_t* bytePtr = nullptr;
    bool openBus = false;

    switch (addr)
    {
        case INT_WAITCNT_ADDR_MIN ... INT_WAITCNT_ADDR_MAX:
            bytePtr = &interruptAndWaitcntRegisters_.at(addr - INT_WAITCNT_ADDR_MIN);
            break;
        case POSTFLG_HALTCNT_ADDR_MIN ... POSTFLG_HALTCNT_ADDR_MAX:
            std::tie(value, openBus) = ReadPostFlgHaltcnt(addr, alignment);
            break;
        case UNDOCUMENTED_ADDR_MIN ... UNDOCUMENTED_ADDR_MAX:
            bytePtr = &undocumentedRegisters_.at(addr - UNDOCUMENTED_ADDR_MIN);
            break;
        case INTERNAL_MEM_CONTROL_ADDR_MIN ... INTERNAL_MEM_CONTROL_ADDR_MAX:
            bytePtr = &internalMemoryControlRegisters_.at(addr - INTERNAL_MEM_CONTROL_ADDR_MIN);
            break;
        default:
            openBus = true;
            break;
    }

    if (bytePtr != nullptr)
    {
        value = ReadPointer(bytePtr, alignment);
    }

    return {value, openBus};
}

void SystemControl::WriteReg(uint32_t addr, uint32_t value, AccessSize alignment)
{
    uint8_t* bytePtr = nullptr;

    switch (addr)
    {
        case INT_WAITCNT_ADDR_MIN ... INT_WAITCNT_ADDR_MAX:
            WriteInterruptWaitcnt(addr, value, alignment);
            break;
        case POSTFLG_HALTCNT_ADDR_MIN ... POSTFLG_HALTCNT_ADDR_MAX:
            CheckHaltWrite(addr, value, alignment);
            bytePtr = &postFlgAndHaltcntRegisters_.at(addr - POSTFLG_HALTCNT_ADDR_MIN);
            break;
        case UNDOCUMENTED_ADDR_MIN ... UNDOCUMENTED_ADDR_MAX:
            bytePtr = &undocumentedRegisters_.at(addr - UNDOCUMENTED_ADDR_MIN);
            break;
        case INTERNAL_MEM_CONTROL_ADDR_MIN ... INTERNAL_MEM_CONTROL_ADDR_MAX:
            bytePtr = &internalMemoryControlRegisters_.at(addr - INTERNAL_MEM_CONTROL_ADDR_MIN);
            break;
        default:
            break;
    }

    if (bytePtr != nullptr)
    {
        WritePointer(bytePtr, value, alignment);
    }

    waitcnt_.gamePakType = 0;
    CheckForInterrupt();
}

void SystemControl::CheckForInterrupt()
{
    if ((ie_ & if_) != 0)
    {
        if (ime_ & 0x01)
        {
            Scheduler.ScheduleEvent(EventType::IRQ, 3);
        }

        halted_ = false;
    }
}

void SystemControl::RequestInterrupt(InterruptType interrupt)
{
    if_ |= static_cast<uint16_t>(interrupt);
    CheckForInterrupt();
}

int SystemControl::WaitStates(WaitState state, bool sequential, AccessSize alignment) const
{
    int firstAccess = 0;
    int secondAccess = 0;

    switch (state)
    {
        case WaitState::ZERO:
        {
            firstAccess = sequential ? SequentialWaitStates[0][waitcnt_.waitState0SecondAccess] :
                                       NonSequentialWaitStates[waitcnt_.waitState0FirstAccess];

            if (alignment == AccessSize::WORD)
            {
                secondAccess = SequentialWaitStates[0][waitcnt_.waitState0SecondAccess];
            }

            break;
        }
        case WaitState::ONE:
        {
            firstAccess = sequential ? SequentialWaitStates[1][waitcnt_.waitState1SecondAccess] :
                                       NonSequentialWaitStates[waitcnt_.waitState1FirstAccess];

            if (alignment == AccessSize::WORD)
            {
                secondAccess = SequentialWaitStates[1][waitcnt_.waitState1SecondAccess];
            }

            break;
        }
        case WaitState::TWO:
        {
            firstAccess = sequential ? SequentialWaitStates[2][waitcnt_.waitState2SecondAccess] :
                                       NonSequentialWaitStates[waitcnt_.waitState2FirstAccess];

            if (alignment == AccessSize::WORD)
            {
                secondAccess = SequentialWaitStates[2][waitcnt_.waitState2SecondAccess];
            }

            break;
        }
        case WaitState::SRAM:
            firstAccess = NonSequentialWaitStates[waitcnt_.sramWaitCtrl];
            break;
    }

    return firstAccess + secondAccess;
}

std::pair<uint32_t, bool> SystemControl::ReadPostFlgHaltcnt(uint32_t addr, AccessSize alignment)
{
    if (((addr == POSTFLG_HALTCNT_ADDR_MIN) && (alignment != AccessSize::BYTE)) || (addr > POSTFLG_HALTCNT_ADDR_MIN))
    {
        return {0, false};
    }

    size_t index = addr - POSTFLG_HALTCNT_ADDR_MIN;
    uint8_t* bytePtr = &postFlgAndHaltcntRegisters_.at(index);
    uint32_t value = ReadPointer(bytePtr, alignment);
    return {value, false};
}

void SystemControl::WriteInterruptWaitcnt(uint32_t addr, uint32_t value, AccessSize alignment)
{
    uint8_t* bytePtr = nullptr;

    if (addr < 0x0400'0204)  // Write to IE and possibly IF
    {
        if (addr < 0x0400'0202)  // Write to IE and possibly IF
        {
            size_t index = addr - INT_WAITCNT_ADDR_MIN;
            bytePtr = &interruptAndWaitcntRegisters_.at(index);

            if (alignment == AccessSize::WORD)  // Regular write to IE and acknowledge interrupt write to IF
            {
                alignment = AccessSize::HALFWORD;
                AcknowledgeInterrupt(value >> 16);
            }
        }
        else  // Write to acknowledge interrupt
        {
            if (alignment == AccessSize::BYTE)
            {
                value = (addr == 0x0400'0202) ? (value & MAX_U8) : ((value & MAX_U8) << 8);
            }

            AcknowledgeInterrupt(value);
        }
    }
    else
    {
        size_t index = addr - INT_WAITCNT_ADDR_MIN;
        bytePtr = &interruptAndWaitcntRegisters_.at(index);
    }

    if (bytePtr != nullptr)
    {
        WritePointer(bytePtr, value, alignment);
    }
}

void SystemControl::AcknowledgeInterrupt(uint16_t acknowledgement)
{
    if_ &= ~acknowledgement;
}

void SystemControl::CheckHaltWrite(uint32_t addr, uint32_t value, AccessSize alignment)
{
    bool haltcntWritten = false;
    uint8_t haltcnt = 0x00;

    if (addr == 0x0400'0300)
    {
        if (alignment != AccessSize::BYTE)
        {
            haltcntWritten = true;
            haltcnt = (value >> 8) & MAX_U8;
        }
    }
    else if (addr == 0x0400'0301)
    {
        haltcntWritten = true;
        haltcnt = value & MAX_U8;
    }

    if (haltcntWritten && !halted_)
    {
        halted_ = (haltcnt & MSB_8) == 0x00;
    }
}
