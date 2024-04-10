#include <System/InterruptManager.hpp>
#include <MemoryMap.hpp>
#include <System/Scheduler.hpp>
#include <cstdint>
#include <stdexcept>
#include <utility>

// Global Interrupt Manager instance
InterruptManager InterruptMgr;

InterruptManager::InterruptManager()
{
    IE_ = 0;
    IF_ = 0;
    IME_ = 0;
}

void InterruptManager::RequestInterrupt(InterruptType interrupt)
{
    IF_ |= static_cast<uint16_t>(interrupt);
    CheckForInterrupt();
}

std::pair<uint32_t, int> InterruptManager::ReadIoReg(uint32_t addr, int accessSize)
{
    // TODO Fix address alignment so that offset reads (ie addr = IE_ADDR - 1, accessSize = 4) work
    // TODO Implement mirroring of UNDOCUMENTED1_ADDR
    uint32_t value;
    int cycles = 1;
    uint32_t bitMask;

    switch (accessSize)
    {
        case 1:
            bitMask = MAX_U8;
            break;
        case 2:
            bitMask = MAX_U16;
            break;
        case 4:
            bitMask = MAX_U32;
            break;
        default:
            throw std::runtime_error("Illegal Read Memory access size");
    }

    switch (addr)
    {
        case IE_ADDR:
            value = IE_ & bitMask;
            break;
        case IF_ADDR:
            value = IF_ & bitMask;
            break;
        case IME_ADDR:
            value = IME_ & bitMask;
            break;
        case POSTFLG_ADDR:
            value = POSTFLG_;
            break;
        case HALTCNT_ADDR:
            value = HALTCNT_;
            break;
        case UNDOCUMENTED0_ADDR:
            value = UNDOCUMENTED0_ & bitMask;
            break;
        case UNDOCUMENTED1_ADDR:
            value = UNDOCUMENTED1_ & bitMask;
            break;
        default:
            value = 0;
            break;
    }

    return {value, cycles};
}

int InterruptManager::WriteIoReg(uint32_t addr, uint32_t value, int accessSize)
{
    // TODO Fix address alignment so that offset writes (ie addr = IE_ADDR - 1, accessSize = 4) work
    // TODO Implement proper interrupt acknowledgement
    // TODO Implement mirroring of UNDOCUMENTED1_ADDR
    switch (addr)
    {
        case IE_ADDR:
            IE_ = (accessSize == 1) ? (IE_ & 0xFF00) | (value & 0x00FF) : value;
            CheckForInterrupt();
            break;
        case IF_ADDR:
            IF_ = (accessSize == 1) ? (IF_ & 0xFF00) | (value & 0x00FF) : value;
            CheckForInterrupt();
            break;
        case IME_ADDR:
            IME_ = (accessSize == 1) ? (IME_ & 0xFF00) | (value & 0x00FF) : value;
            CheckForInterrupt();
            break;
        case POSTFLG_ADDR:
            POSTFLG_ = value;
            break;
        case HALTCNT_ADDR:
            HALTCNT_ = value;
            break;
        case UNDOCUMENTED0_ADDR:
            UNDOCUMENTED0_ = value;
            break;
        case UNDOCUMENTED1_ADDR:
            UNDOCUMENTED1_ = value;
            break;
        default:
            break;
    }

    return 1;
}

void InterruptManager::CheckForInterrupt() const
{
    if ((IME_ & 0x01) && ((IF_ & IE_) != 0))
    {
        Scheduler.ScheduleEvent(EventType::IRQ, SCHEDULE_NOW);
    }
}
