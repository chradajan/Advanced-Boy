#include <System/GameBoyAdvance.hpp>
#include <array>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <Audio/SoundController.hpp>
#include <Logging/Logging.hpp>
#include <System/MemoryMap.hpp>
#include <System/EventScheduler.hpp>
#include <System/SystemControl.hpp>
#include <Timers/TimerManager.hpp>
#include <Utilities/Functor.hpp>
#include <Utilities/MemoryUtilities.hpp>

namespace fs = std::filesystem;

GameBoyAdvance::GameBoyAdvance(fs::path biosPath) :
    biosLoaded_(LoadBIOS(biosPath)),
    cpu_(CPU::MemReadCallback(&ReadMemory, *this), CPU::MemWriteCallback(&WriteMemory, *this)),
    gamePak_(nullptr),
    dmaChannels_({0, 1, 2, 3})
{
    // State
    gamePakLoaded_ = false;

    Scheduler.RegisterEvent(EventType::HBlank, std::bind(&HBlank, this, std::placeholders::_1), true);
    Scheduler.RegisterEvent(EventType::VBlank, std::bind(&VBlank, this, std::placeholders::_1), true);
    Scheduler.RegisterEvent(EventType::DMA0, std::bind(&DMA0, this, std::placeholders::_1), false);
    Scheduler.RegisterEvent(EventType::DMA1, std::bind(&DMA1, this, std::placeholders::_1), false);
    Scheduler.RegisterEvent(EventType::DMA2, std::bind(&DMA2, this, std::placeholders::_1), false);
    Scheduler.RegisterEvent(EventType::DMA3, std::bind(&DMA3, this, std::placeholders::_1), false);
    Scheduler.RegisterEvent(EventType::Timer0Overflow, std::bind(&Timer0Overflow, this, std::placeholders::_1), true);
    Scheduler.RegisterEvent(EventType::Timer1Overflow, std::bind(&Timer1Overflow, this, std::placeholders::_1), true);
}

void GameBoyAdvance::Reset()
{
    // Scheduler
    Scheduler.Reset();

    // Components
    apu_.Reset();
    cpu_.Reset();
    gamepad_.Reset();
    ppu_.Reset();
    timerManager_.Reset();
    SystemController.Reset();

    if (gamePakLoaded_)
    {
        gamePak_->Reset();
    }

    // DMA
    dmaImmediately_.fill(false);
    dmaOnVBlank_.fill(false);
    dmaOnHBlank_.fill(false);
    dmaOnReplenishA_.fill(false);
    dmaOnReplenishB_.fill(false);
    activeDmaChannel_ = {};

    for (auto& channel : dmaChannels_)
    {
        channel.Reset();
    }

    // Memory
    onBoardWRAM_.fill(0);
    onChipWRAM_.fill(0);
    placeholderIoRegisters_.fill(0);

    // Open bus
    lastBiosFetch_ = 0;
    lastReadValue_ = 0;

    // Scheduler
    Scheduler.ScheduleEvent(EventType::HBlank, 960);
}

void GameBoyAdvance::FillAudioBuffer(int samples)
{
    if (!biosLoaded_ && !gamePakLoaded_)
    {
        return;
    }

    try
    {
        while (apu_.SampleCount() < samples)
        {
            if (SystemController.Halted() || activeDmaChannel_.has_value())
            {
                Scheduler.SkipToNextEvent();
            }
            else
            {
                cpu_.Step();
                Scheduler.CheckEventQueue();
            }
        }
    }
    catch (std::exception const& error)
    {
        Logging::LogMgr.LogException(error);
        Logging::LogMgr.DumpLogs();
        throw;
    }
}

void GameBoyAdvance::DrainAudioBuffer(float* buffer)
{
    apu_.DrainInternalBuffer(buffer);
}

std::pair<uint32_t, int> GameBoyAdvance::ReadMemory(uint32_t addr, AccessSize alignment)
{
    addr = AlignAddress(addr, alignment);
    uint32_t value = 0;
    int cycles = 1;
    bool openBus = false;
    auto page = MemoryPage::INVALID;

    if (addr < 0x1000'0000)
    {
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wnarrowing"
        page = MemoryPage{(addr & 0x0F00'0000) >> 24};
        #pragma GCC diagnostic pop
    }

    switch (page)
    {
        case MemoryPage::BIOS:
            std::tie(value, cycles) = ReadBIOS(addr, alignment);
            break;
        case MemoryPage::WRAM_SLOW:
            std::tie(value, cycles) = ReadOnBoardWRAM(addr, alignment);
            break;
        case MemoryPage::WRAM_FAST:
            std::tie(value, cycles) = ReadOnChipWRAM(addr, alignment);
            break;
        case MemoryPage::IO_REG:
            std::tie(value, cycles) = ReadIoReg(addr, alignment);
            break;
        case MemoryPage::PRAM:
            std::tie(value, cycles) = ppu_.ReadPRAM(addr, alignment);
            break;
        case MemoryPage::VRAM:
            std::tie(value, cycles) = ppu_.ReadVRAM(addr, alignment);
            break;
        case MemoryPage::OAM:
            std::tie(value, cycles) = ppu_.ReadOAM(addr, alignment);
            break;
        case MemoryPage::GAMEPAK_MIN ... MemoryPage::GAMEPAK_MAX:
            std::tie(value, cycles, openBus) = gamePak_->ReadGamePak(addr, alignment);
            break;
        case MemoryPage::INVALID:
        default:
            openBus = true;
            break;
    }

    if (openBus)
    {
        std::tie(value, cycles) = ReadOpenBus(addr, alignment);
    }

    lastReadValue_ = value;
    return {value, cycles};
}

int GameBoyAdvance::WriteMemory(uint32_t addr, uint32_t value, AccessSize alignment)
{
    addr = AlignAddress(addr, alignment);
    int cycles = 1;
    auto page = MemoryPage::INVALID;

    if (addr < 0x1000'0000)
    {
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wnarrowing"
        page = MemoryPage{(addr & 0x0F00'0000) >> 24};
        #pragma GCC diagnostic pop
    }

    switch (page)
    {
        case MemoryPage::BIOS:
            break;
        case MemoryPage::WRAM_SLOW:
            cycles = WriteOnBoardWRAM(addr, value, alignment);
            break;
        case MemoryPage::WRAM_FAST:
            cycles = WriteOnChipWRAM(addr, value, alignment);
            break;
        case MemoryPage::IO_REG:
            cycles = WriteIoReg(addr, value, alignment);
            break;
        case MemoryPage::PRAM:
            cycles = ppu_.WritePRAM(addr, value, alignment);
            break;
        case MemoryPage::VRAM:
            cycles = ppu_.WriteVRAM(addr, value, alignment);
            break;
        case MemoryPage::OAM:
            cycles = ppu_.WriteOAM(addr, value, alignment);
            break;
        case MemoryPage::GAMEPAK_MIN ... MemoryPage::GAMEPAK_MAX:
            cycles = gamePak_->WriteGamePak(addr, value, alignment);
            break;
        case MemoryPage::INVALID:
        default:
            break;
    }

    return cycles;
}

bool GameBoyAdvance::LoadGamePak(fs::path romPath)
{
    gamePak_.reset();
    gamePak_ = std::make_unique<Cartridge::GamePak>(romPath);
    gamePakLoaded_ = gamePak_->RomLoaded();

    if (gamePakLoaded_)
    {
        Reset();
    }

    return gamePakLoaded_;
}

void GameBoyAdvance::UpdateGamepad(Gamepad gamepad)
{
    gamepad_.UpdateGamepad(gamepad);
}

void GameBoyAdvance::DumpLogs() const
{
    Logging::LogMgr.DumpLogs();
}

std::string GameBoyAdvance::RomTitle() const
{
    if (gamePakLoaded_)
    {
        return gamePak_->RomTitle();
    }

    return "";
}

bool GameBoyAdvance::LoadBIOS(fs::path biosPath)
{
    if (biosPath.empty())
    {
        return false;
    }

    auto fileSizeInBytes = fs::file_size(biosPath);

    if (fileSizeInBytes != BIOS_.size())
    {
        return false;
    }

    std::ifstream bios(biosPath, std::ios::binary);

    if (bios.fail())
    {
        return false;
    }

    bios.read(reinterpret_cast<char*>(BIOS_.data()), fileSizeInBytes);
    return true;
}

void GameBoyAdvance::HBlank(int extraCycles)
{
    ppu_.HBlank(extraCycles);

    if (ppu_.CurrentScanline() < 160)
    {
        ScheduleDMA(dmaOnHBlank_);
    }
}

void GameBoyAdvance::VBlank(int extraCycles)
{
    ppu_.VBlank(extraCycles);

    if (ppu_.CurrentScanline() == 160)
    {
        ScheduleDMA(dmaOnVBlank_);
    }
}

void GameBoyAdvance::TimerOverflow(int timer, int extraCycles)
{
    switch (timer)
    {
        case 0:
            timerManager_.Timer0Overflow(extraCycles);
            break;
        case 1:
            timerManager_.Timer1Overflow(extraCycles);
            break;
        default:
            return;
    }

    auto [replenishA, replenishB] = apu_.UpdateDmaSound(timer);

    if (replenishA)
    {
        ScheduleDMA(dmaOnReplenishA_);
    }

    if (replenishB)
    {
        ScheduleDMA(dmaOnReplenishB_);
    }
}

void GameBoyAdvance::ExecuteDMA(int dmaChannelIndex)
{
    dmaChannels_[dmaChannelIndex].Execute(this);
    activeDmaChannel_ = {};

    for (int nextActiveChannel = dmaChannelIndex + 1; nextActiveChannel < 4; ++nextActiveChannel)
    {
        if (Scheduler.EventScheduled(dmaChannels_[nextActiveChannel].GetDmaEvent()))
        {
            activeDmaChannel_ = nextActiveChannel;
            break;
        }
    }
}

void GameBoyAdvance::ScheduleDMA(std::array<bool, 4>& enabledChannels)
{
    for (int channelIndex = 0; channelIndex < 4; ++channelIndex)
    {
        // Do nothing on channels not triggered to start on the current event or ones already scheduled to run.
        if (!enabledChannels[channelIndex] || Scheduler.EventScheduled(dmaChannels_[channelIndex].GetDmaEvent()))
        {
            continue;
        }

        if (!activeDmaChannel_.has_value())
        {
            // Immediately start the DMA channel at the current index if no other channels are actively running
            activeDmaChannel_ = channelIndex;
            EventType dmaEvent = dmaChannels_[channelIndex].GetDmaEvent();
            int cyclesUntilEvent = dmaChannels_[channelIndex].TransferCycleCount(this);
            Scheduler.ScheduleEvent(dmaEvent, cyclesUntilEvent);
        }
        else if (activeDmaChannel_.value() < channelIndex)
        {
            // Don't start this channel until all higher priority channels have completed.
            EventType dmaEvent = dmaChannels_[channelIndex].GetDmaEvent();
            int cyclesUntilEvent = dmaChannels_[channelIndex].TransferCycleCount(this);

            for (int higherPriorityIndex = channelIndex - 1; higherPriorityIndex >= 0; --higherPriorityIndex)
            {
                auto higherPriorityRemainingCycles = Scheduler.CyclesRemaining(dmaChannels_[higherPriorityIndex].GetDmaEvent());

                if (higherPriorityRemainingCycles.has_value())
                {
                    cyclesUntilEvent += higherPriorityRemainingCycles.value();
                    break;
                }
            }

            Scheduler.ScheduleEvent(dmaEvent, cyclesUntilEvent);
        }
        else
        {
            // Event and timing for current channel index
            EventType newDmaEvent = dmaChannels_[channelIndex].GetDmaEvent();
            int cyclesUntilNewEvent = dmaChannels_[channelIndex].TransferCycleCount(this);

            // Currently running DMA channel info
            int currentActiveIndex = activeDmaChannel_.value();
            EventType currentActiveEvent = dmaChannels_[activeDmaChannel_.value()].GetDmaEvent();
            int currentActiveCatchUpCycles = Scheduler.ElapsedCycles(currentActiveEvent).value();
            int currentActiveRemainingCycles = Scheduler.CyclesRemaining(currentActiveEvent).value();

            // Catch up and reschedule current channel
            dmaChannels_[activeDmaChannel_.value()].PartiallyExecute(this, currentActiveCatchUpCycles);
            Scheduler.UnscheduleEvent(currentActiveEvent);
            Scheduler.ScheduleEvent(currentActiveEvent, cyclesUntilNewEvent + currentActiveRemainingCycles);

            // Push back any other lower priority channels that were waiting on the currently running channel
            int higherPriorityCyclesRemaining = cyclesUntilNewEvent + currentActiveRemainingCycles;

            for (int channelToPushback = currentActiveIndex + 1; channelToPushback < 4; ++channelToPushback)
            {
                EventType pushedBackEvent = dmaChannels_[channelToPushback].GetDmaEvent();
                auto pushedBackEventLength = Scheduler.EventLength(pushedBackEvent);

                if (pushedBackEventLength.has_value())
                {
                    Scheduler.UnscheduleEvent(pushedBackEvent);
                    Scheduler.ScheduleEvent(pushedBackEvent, higherPriorityCyclesRemaining + pushedBackEventLength.value());
                    higherPriorityCyclesRemaining += pushedBackEventLength.value();
                }
            }

            // Mark the current channel index as active and schedule it
            activeDmaChannel_ = channelIndex;
            Scheduler.ScheduleEvent(newDmaEvent, cyclesUntilNewEvent);
        }
    }
}

std::pair<uint32_t, int> GameBoyAdvance::ReadBIOS(uint32_t addr, AccessSize alignment)
{
    uint32_t value = 0;
    int cycles = 1;

    if (addr <= BIOS_ADDR_MAX)
    {
        if (cpu_.GetPC() <= BIOS_ADDR_MAX)
        {
            size_t index = addr - BIOS_ADDR_MIN;
            uint8_t* bytePtr = &BIOS_.at(index);
            value = ReadPointer(bytePtr, alignment);
            lastBiosFetch_ = value;
        }
        else
        {
            value = lastBiosFetch_;
        }
    }
    else
    {
        std::tie(value, cycles) = ReadOpenBus(addr, alignment);
    }

    return {value, cycles};
}

std::pair<uint32_t, int> GameBoyAdvance::ReadOnBoardWRAM(uint32_t addr, AccessSize alignment)
{
    if (addr > WRAM_ON_BOARD_ADDR_MAX)
    {
        addr = WRAM_ON_BOARD_ADDR_MIN + (addr % onBoardWRAM_.size());
    }

    size_t index = addr - WRAM_ON_BOARD_ADDR_MIN;
    uint8_t* bytePtr = &onBoardWRAM_.at(index);
    uint32_t value = ReadPointer(bytePtr, alignment);
    int cycles = (alignment == AccessSize::WORD) ? 6 : 3;
    return {value, cycles};
}

int GameBoyAdvance::WriteOnBoardWRAM(uint32_t addr, uint32_t value, AccessSize alignment)
{
    if (addr > WRAM_ON_BOARD_ADDR_MAX)
    {
        addr = WRAM_ON_BOARD_ADDR_MIN + (addr % onBoardWRAM_.size());
    }

    size_t index = addr - WRAM_ON_BOARD_ADDR_MIN;
    uint8_t* bytePtr = &onBoardWRAM_.at(index);
    WritePointer(bytePtr, value, alignment);
    return (alignment == AccessSize::WORD) ? 6 : 3;
}

std::pair<uint32_t, int> GameBoyAdvance::ReadOnChipWRAM(uint32_t addr, AccessSize alignment)
{
    if (addr > WRAM_ON_CHIP_ADDR_MAX)
    {
        addr = WRAM_ON_CHIP_ADDR_MIN + (addr % onChipWRAM_.size());
    }

    size_t index = addr - WRAM_ON_CHIP_ADDR_MIN;
    uint8_t* bytePtr = &onChipWRAM_.at(index);
    uint32_t value = ReadPointer(bytePtr, alignment);
    return {value, 1};
}

int GameBoyAdvance::WriteOnChipWRAM(uint32_t addr, uint32_t value, AccessSize alignment)
{
    if (addr > WRAM_ON_CHIP_ADDR_MAX)
    {
        addr = WRAM_ON_CHIP_ADDR_MIN + (addr % onChipWRAM_.size());
    }

    size_t index = addr - WRAM_ON_CHIP_ADDR_MIN;
    uint8_t* bytePtr = &onChipWRAM_.at(index);
    WritePointer(bytePtr, value, alignment);
    return 1;
}

std::pair<uint32_t, int> GameBoyAdvance::ReadIoReg(uint32_t addr, AccessSize alignment)
{
    if ((addr > INT_WTST_PWRDWN_IO_ADDR_MAX) && ((addr % (64 * KiB) < 4)))
    {
        addr = 0x0400'0800 + (addr % (64 * KiB));
    }

    uint32_t value = 0;
    int cycles = 1;
    bool openBus = false;
    bool unhandledRegion = false;

    switch (addr)
    {
        case LCD_IO_ADDR_MIN ... LCD_IO_ADDR_MAX:
            std::tie(value, openBus) = ppu_.ReadReg(addr, alignment);
            break;
        case SOUND_IO_ADDR_MIN ... SOUND_IO_ADDR_MAX:
            std::tie(value, openBus) = apu_.ReadReg(addr, alignment);
            break;
        case DMA_TRANSFER_CHANNELS_IO_ADDR_MIN ... DMA_TRANSFER_CHANNELS_IO_ADDR_MAX:
            std::tie(value, openBus) = ReadDmaReg(addr, alignment);
            break;
        case TIMER_IO_ADDR_MIN ... TIMER_IO_ADDR_MAX:
            std::tie(value, openBus) = timerManager_.ReadReg(addr, alignment);
            break;
        case SERIAL_COMMUNICATION_1_IO_ADDR_MIN ... SERIAL_COMMUNICATION_1_IO_ADDR_MAX:
            unhandledRegion = true;
            break;
        case KEYPAD_INPUT_IO_ADDR_MIN ... KEYPAD_INPUT_IO_ADDR_MAX:
            std::tie(value, openBus) = gamepad_.ReadReg(addr, alignment);
            break;
        case SERIAL_COMMUNICATION_2_IO_ADDR_MIN ... SERIAL_COMMUNICATION_2_IO_ADDR_MAX:
            unhandledRegion = true;
            break;
        case INT_WTST_PWRDWN_IO_ADDR_MIN ... INT_WTST_PWRDWN_IO_ADDR_MAX:
            std::tie(value, openBus) = SystemController.ReadReg(addr, alignment);
            break;
        default:
            openBus = true;
            break;
    }

    if (unhandledRegion)
    {
        size_t index = addr - IO_REG_ADDR_MIN;
        uint8_t* bytePtr = &placeholderIoRegisters_.at(index);
        value = ReadPointer(bytePtr, alignment);
    }
    else if (openBus)
    {
        std::tie(value, cycles) = ReadOpenBus(addr, alignment);
    }

    return {value, cycles};
}

int GameBoyAdvance::WriteIoReg(uint32_t addr, uint32_t value, AccessSize alignment)
{
    if ((addr > INT_WTST_PWRDWN_IO_ADDR_MAX) && ((addr % (64 * KiB) < 4)))
    {
        addr = 0x0400'0800 + (addr % (64 * KiB));
    }

    bool unhandledRegion = false;

    switch (addr)
    {
        case LCD_IO_ADDR_MIN ... LCD_IO_ADDR_MAX:
            ppu_.WriteReg(addr, value, alignment);
            break;
        case SOUND_IO_ADDR_MIN ... SOUND_IO_ADDR_MAX:
            apu_.WriteReg(addr, value, alignment);
            break;
        case DMA_TRANSFER_CHANNELS_IO_ADDR_MIN ... DMA_TRANSFER_CHANNELS_IO_ADDR_MAX:
            WriteDmaReg(addr, value, alignment);
            break;
        case TIMER_IO_ADDR_MIN ... TIMER_IO_ADDR_MAX:
            timerManager_.WriteReg(addr, value, alignment);
            break;
        case SERIAL_COMMUNICATION_1_IO_ADDR_MIN ... SERIAL_COMMUNICATION_1_IO_ADDR_MAX:
            unhandledRegion = true;
            break;
        case KEYPAD_INPUT_IO_ADDR_MIN ... KEYPAD_INPUT_IO_ADDR_MAX:
            gamepad_.WriteReg(addr, value, alignment);
            break;
        case SERIAL_COMMUNICATION_2_IO_ADDR_MIN ... SERIAL_COMMUNICATION_2_IO_ADDR_MAX:
            unhandledRegion = true;
            break;
        case INT_WTST_PWRDWN_IO_ADDR_MIN ... INT_WTST_PWRDWN_IO_ADDR_MAX:
            SystemController.WriteReg(addr, value, alignment);
            break;
        default:
            break;
    }

    if (unhandledRegion)
    {
        size_t index = addr - IO_REG_ADDR_MIN;
        uint8_t* bytePtr = &placeholderIoRegisters_.at(index);
        WritePointer(bytePtr, value, alignment);
    }

    return 1;
}

std::pair<uint32_t, bool> GameBoyAdvance::ReadDmaReg(uint32_t addr, AccessSize alignment)
{
    if (addr > 0x0400'00DF)
    {
        return {0, true};
    }

    size_t channelIndex = (addr - DMA_TRANSFER_CHANNELS_IO_ADDR_MIN) / 12;
    return dmaChannels_[channelIndex].ReadReg(addr, alignment);
}

void GameBoyAdvance::WriteDmaReg(uint32_t addr, uint32_t value, AccessSize alignment)
{
    if (addr > 0x0400'00DF)
    {
        return;
    }

    size_t channelIndex = (addr - DMA_TRANSFER_CHANNELS_IO_ADDR_MIN) / 12;
    dmaChannels_[channelIndex].WriteReg(addr, value, alignment, this);
}

std::pair<uint32_t, int> GameBoyAdvance::ReadOpenBus(uint32_t addr, AccessSize alignment)
{
    uint32_t value = lastReadValue_;
    (void)addr;

    switch (alignment)
    {
        case AccessSize::BYTE:
            value = lastReadValue_ & MAX_U8;
            break;
        case AccessSize::HALFWORD:
            value = lastReadValue_ & MAX_U16;
            break;
        case AccessSize::WORD:
            value = lastReadValue_;
            break;
    }

    return {value, 1};
}
