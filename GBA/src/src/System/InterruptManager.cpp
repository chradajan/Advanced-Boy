#include <System/InterruptManager.hpp>
#include <cstdint>
#include <stdexcept>
#include <tuple>
#include <System/MemoryMap.hpp>
#include <System/Scheduler.hpp>
#include <System/Utilities.hpp>

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
    halted_ = false;
}

void InterruptManager::RequestInterrupt(InterruptType interrupt)
{
    IF_ |= static_cast<uint16_t>(interrupt);
    CheckForInterrupt();
}

std::tuple<uint32_t, int, bool> InterruptManager::ReadIoReg(uint32_t addr, AccessSize alignment)
{
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
    uint32_t maxAddrWritten = addr + static_cast<uint8_t>(alignment) - 1;
    size_t index = addr - INT_WTST_PWRDWN_IO_ADDR_MIN;
    uint8_t* bytePtr = &(intWtstPwdDownRegisters_.at(index));

    bool acknowledgingIRQ = (addr <= IF_ADDR) && (IF_ADDR <= maxAddrWritten);
    bool writingHALTCNT = (addr <= HALTCNT_ADDR) && (HALTCNT_ADDR <= maxAddrWritten);

    if (acknowledgingIRQ)
    {
        switch (alignment)
        {
            case AccessSize::BYTE:
            {
                // Byte alignment means only one byte of IF is affected.
                if (addr == IF_ADDR)
                {
                    // Write to lower byte.
                    IF_ &= ~(value & MAX_U8);
                }
                else
                {
                    // Write to upper byte.
                    IF_ &= ~((value & MAX_U8) << 8);
                }

                break;
            }
            case AccessSize::HALFWORD:
                // Halfword alignment means all of IF is affected.
                IF_ &= ~(value & MAX_U16);
                break;
            case AccessSize::WORD:
            {
                // IF is halfword aligned so a word write means IE is being written to.
                uint16_t newIE = value & MAX_U16;
                uint16_t ackMask = value >> 16;
                IE_ = newIE;
                IF_ &= ~ackMask;
                break;
            }
        }
    }
    else
    {
        WritePointer(bytePtr, value, alignment);
    }

    if (writingHALTCNT)
    {
        if (HALTCNT_ & 0x80)
        {
            // Stop
        }
        else
        {
            // Halt
            if ((IF_ & IE_) == 0)
            {
                halted_ = true;
                Scheduler.ScheduleEvent(EventType::Halt, SCHEDULE_NOW);
            }
        }
    }
    else
    {
        CheckForInterrupt();
    }

    return 1;
}

void InterruptManager::CheckForInterrupt()
{
    if ((IF_ & IE_) != 0)
    {
        if (IME_ & 0x01)
        {
            Scheduler.ScheduleEvent(EventType::IRQ, SCHEDULE_NOW);
        }
        else if (halted_)
        {
            Scheduler.ScheduleEvent(EventType::Halt, SCHEDULE_NOW);
        }

        halted_ = false;
    }
}
