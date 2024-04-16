#include <System/InterruptManager.hpp>
#include <System/MemoryMap.hpp>
#include <System/Scheduler.hpp>
#include <System/Utilities.hpp>
#include <cstdint>
#include <stdexcept>
#include <tuple>

// Global Interrupt Manager instance
InterruptManager InterruptMgr;

InterruptManager::InterruptManager() :
    intWtstPwdDownRegisters_(),
    IE_(*reinterpret_cast<uint16_t*>(&intWtstPwdDownRegisters_.at(0))),
    IF_(*reinterpret_cast<uint16_t*>(&intWtstPwdDownRegisters_.at(2))),
    IME_(*reinterpret_cast<uint16_t*>(&intWtstPwdDownRegisters_.at(8))),
    POSTFLG_(intWtstPwdDownRegisters_.at(256)),
    HALTCNT_(intWtstPwdDownRegisters_.at(257))
{
    intWtstPwdDownRegisters_.fill(0);
}

void InterruptManager::RequestInterrupt(InterruptType interrupt)
{
    IF_ |= static_cast<uint16_t>(interrupt);
    CheckForInterrupt();
}

std::tuple<uint32_t, int, bool> InterruptManager::ReadIoReg(uint32_t addr, AccessSize alignment)
{
    addr = AlignAddress(addr, alignment);
    std::tuple<uint32_t, int, bool> openBusCondition = {0, 1, true};
    std::tuple<uint32_t, int, bool> zeroCondition = {0, 1, false};

    if ((addr >= WAITCNT_ADDR) && (addr < IME_ADDR))
    {
        if ((addr >= 0x0400'0206))
        {
            return openBusCondition;
        }
        else if (alignment == AccessSize::WORD)
        {
            return zeroCondition;
        }
    }
    else if ((addr >= IME_ADDR) && (addr < POSTFLG_ADDR))
    {
        if (addr >= 0x0400'020A)
        {
            return openBusCondition;
        }
        else if (alignment == AccessSize::WORD)
        {
            return zeroCondition;
        }
    }
    else if ((addr >= POSTFLG_ADDR) && (addr < MIRRORED_IO_ADDR))
    {
        if (addr >= 0x0400'0302)
        {
            return openBusCondition;
        }
        else if ((alignment != AccessSize::BYTE) || (addr == HALTCNT_ADDR))
        {
            return zeroCondition;
        }
    }

    size_t index = addr - INT_WTST_PWRDWN_IO_ADDR_MIN;
    uint8_t* bytePtr = &(intWtstPwdDownRegisters_.at(index));
    uint32_t value = ReadPointer(bytePtr, alignment);
    return {value, 1, false};
}

int InterruptManager::WriteIoReg(uint32_t addr, uint32_t value, AccessSize alignment)
{
    addr = AlignAddress(addr, alignment);
    size_t index = addr - INT_WTST_PWRDWN_IO_ADDR_MIN;
    uint8_t* bytePtr = &(intWtstPwdDownRegisters_.at(index));
    WritePointer(bytePtr, value, alignment);
    CheckForInterrupt();
    return 1;
}

void InterruptManager::CheckForInterrupt() const
{
    if ((IME_ & 0x01) && ((IF_ & IE_) != 0))
    {
        Scheduler.ScheduleEvent(EventType::IRQ, SCHEDULE_NOW);
    }
}
