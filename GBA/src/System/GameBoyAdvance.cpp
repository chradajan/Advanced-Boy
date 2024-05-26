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
#include <Config.hpp>
#include <Logging/Logging.hpp>
#include <System/MemoryMap.hpp>
#include <System/InterruptManager.hpp>
#include <System/EventScheduler.hpp>
#include <Timers/TimerManager.hpp>
#include <Utilities/Functor.hpp>
#include <Utilities/MemoryUtilities.hpp>

namespace fs = std::filesystem;

GameBoyAdvance::GameBoyAdvance(fs::path biosPath) :
    biosLoaded_(LoadBIOS(biosPath)),
    cpu_(CPU::MemReadCallback(&ReadMemory, *this), CPU::MemWriteCallback(&WriteMemory, *this)),
    ppu_(paletteRAM_, VRAM_, OAM_),
    gamePak_(nullptr),
    dmaChannels_({0, 1, 2, 3})
{
    // State
    gamePakLoaded_ = false;
    halted_ = false;

    // DMA
    dmaImmediately_.fill(false);
    dmaOnVBlank_.fill(false);
    dmaOnHBlank_.fill(false);
    dmaOnReplenishA_.fill(false);
    dmaOnReplenishB_.fill(false);
    activeDmaChannel_ = {};

    // Open bus
    lastBiosFetch_ = 0;
    lastReadValue_ = 0;

    Scheduler.RegisterEvent(EventType::Halt, std::bind(&Halt, this, std::placeholders::_1), false);
    Scheduler.RegisterEvent(EventType::HBlank, std::bind(&HBlank, this, std::placeholders::_1), true);
    Scheduler.RegisterEvent(EventType::VBlank, std::bind(&VBlank, this, std::placeholders::_1), true);
    Scheduler.RegisterEvent(EventType::DMA0, std::bind(&DMA0, this, std::placeholders::_1), false);
    Scheduler.RegisterEvent(EventType::DMA1, std::bind(&DMA1, this, std::placeholders::_1), false);
    Scheduler.RegisterEvent(EventType::DMA2, std::bind(&DMA2, this, std::placeholders::_1), false);
    Scheduler.RegisterEvent(EventType::DMA3, std::bind(&DMA3, this, std::placeholders::_1), false);
    Scheduler.RegisterEvent(EventType::Timer0Overflow, std::bind(&Timer0Overflow, this, std::placeholders::_1), true);
    Scheduler.RegisterEvent(EventType::Timer1Overflow, std::bind(&Timer1Overflow, this, std::placeholders::_1), true);

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
            if (halted_ || activeDmaChannel_.has_value())
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
    uint8_t page = (addr & 0x0F00'0000) >> 24;
    uint32_t value = 0;
    int cycles = 1;
    bool openBus = false;

    if (addr & 0xF000'0000)
    {
        // Addresses above 0x0FFF'FFFF are unused and trigger open bus. Set to unused page to skip switch case.
        page = 0x01;
    }

    switch (page)
    {
        case 0x00:  // BIOS
        {
            if (addr <= BIOS_ADDR_MAX)
            {
                std::tie(value, cycles) = ReadBIOS(addr, alignment);
            }
            else
            {
                openBus = true;
            }

            break;
        }
        case 0x02:  // WRAM - On-board
            std::tie(value, cycles) = ReadOnBoardWRAM(addr, alignment);
            break;
        case 0x03:  // WRAM - On-chip
            std::tie(value, cycles) = ReadOnChipWRAM(addr, alignment);
            break;
        case 0x04:  // I/O Registers
        {
            cycles = 1;

            if ((addr <= IO_REG_ADDR_MAX) || (((addr % 0x0001'0000) - 0x0800) < 4))
            {
                std::tie(value, openBus) = ReadIoReg(addr, alignment);
            }
            else
            {
                openBus = true;
            }

            break;
        }
        case 0x05:  // Palette RAM
            std::tie(value, cycles) = ReadPaletteRAM(addr, alignment);
            break;
        case 0x06:  // VRAM
            std::tie(value, cycles) = ReadVRAM(addr, alignment);
            break;
        case 0x07:  // OAM
            std::tie(value, cycles) = ReadOAM(addr, alignment);
            break;
        case 0x08 ... 0x0F:  // GamePak
        {
            if (addr <= GAME_PAK_ADDR_MAX)
            {
                std::tie(value, cycles, openBus) = gamePak_->ReadGamePak(addr, alignment);
            }
            else
            {
                openBus = true;
            }

            break;
        }
        default:
            openBus = true;
            break;
    }

    if (openBus)
    {
        std::tie(value, cycles) = ReadOpenBus(addr, alignment);
    }
    else
    {
        lastReadValue_ = value;
    }

    return {value, cycles};
}

int GameBoyAdvance::WriteMemory(uint32_t addr, uint32_t value, AccessSize alignment)
{
    uint8_t page = (addr & 0x0F00'0000) >> 24;
    int cycles = 1;

    if (addr & 0xF000'0000)
    {
        // Addresses above 0x0FFF'FFFF are unused and trigger open bus. Set to unused page to skip switch case.
        page = 0x01;
    }

    switch (page)
    {
        case 0x00:  // BIOS ROM
            break;
        case 0x02:  // WRAM - On-board
            cycles = WriteOnBoardWRAM(addr, value, alignment);
            break;
        case 0x03:  // WRAM - On-chip
            cycles = WriteOnChipWRAM(addr, value, alignment);
            break;
        case 0x04:  // I/O Registers
        {
            cycles = 1;

            if ((addr <= IO_REG_ADDR_MAX) || (((addr % 0x0001'0000) - 0x0800) < 4))
            {
                WriteIoReg(addr, value, alignment);
            }

            break;
        }
        case 0x05:  // Palette RAM
            cycles = WritePaletteRAM(addr, value, alignment);
            break;
        case 0x06:  // VRAM
            cycles = WriteVRAM(addr, value, alignment);
            break;
        case 0x07:  // OAM
            cycles = WriteOAM(addr, value, alignment);
            break;
        case 0x08 ... 0x0F:  // GamePak
        {
            if (addr <= GAME_PAK_ADDR_MAX)
            {
                cycles = gamePak_->WriteGamePak(addr, value, alignment);
            }

            break;
        }
        default:
            cycles = 1;
            break;
    }

    return cycles;
}

bool GameBoyAdvance::LoadGamePak(fs::path romPath)
{
    gamePak_.reset();
    gamePak_ = std::make_unique<Cartridge::GamePak>(romPath);
    ZeroMemory();
    gamePakLoaded_ = gamePak_->RomLoaded();
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

void GameBoyAdvance::ZeroMemory()
{
    onBoardWRAM_.fill(0);
    onChipWRAM_.fill(0);
    paletteRAM_.fill(0);
    VRAM_.fill(0);
    OAM_.fill(0);
    placeholderIoRegisters_.fill(0);
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

//                Bus   Read      Write     Cycles
//  BIOS ROM      32    8/16/32   -         1/1/1

std::pair<uint32_t, int> GameBoyAdvance::ReadBIOS(uint32_t addr, AccessSize alignment)
{
    if (cpu_.GetPC() <= BIOS_ADDR_MAX)
    {
        addr = AlignAddress(addr, alignment);
        size_t index = addr - BIOS_ADDR_MIN;
        uint8_t* bytePtr = &(BIOS_.at(index));
        uint32_t value = ReadPointer(bytePtr, alignment);
        lastBiosFetch_ = value;
        return {value, 1};
    }
    else
    {
        return {lastBiosFetch_, 1};
    }
}

//  Region        Bus   Read      Write     Cycles
//  Work RAM 256K 16    8/16/32   8/16/32   3/3/6 **

std::pair<uint32_t, int> GameBoyAdvance::ReadOnBoardWRAM(uint32_t addr, AccessSize alignment)
{
    if (addr > WRAM_ON_BOARD_ADDR_MAX)
    {
        addr = WRAM_ON_BOARD_ADDR_MIN + (addr % (256 * KiB));
    }

    addr = AlignAddress(addr, alignment);
    size_t index = addr - WRAM_ON_BOARD_ADDR_MIN;
    uint8_t* bytePtr = &(onBoardWRAM_.at(index));
    uint32_t value = ReadPointer(bytePtr, alignment);
    return {value, alignment == AccessSize::WORD ? 6 : 3};
}

int GameBoyAdvance::WriteOnBoardWRAM(uint32_t addr, uint32_t value, AccessSize alignment)
{
    if (addr > WRAM_ON_BOARD_ADDR_MAX)
    {
        addr = WRAM_ON_BOARD_ADDR_MIN + (addr % (256 * KiB));
    }

    addr = AlignAddress(addr, alignment);
    size_t index = addr - WRAM_ON_BOARD_ADDR_MIN;
    uint8_t* bytePtr = &(onBoardWRAM_.at(index));
    WritePointer(bytePtr, value, alignment);
    return (alignment == AccessSize::WORD ? 6 : 3);
}

//                Bus   Read      Write     Cycles
//  Work RAM 32K  32    8/16/32   8/16/32   1/1/1

std::pair<uint32_t, int> GameBoyAdvance::ReadOnChipWRAM(uint32_t addr, AccessSize alignment)
{
    if (addr > WRAM_ON_CHIP_ADDR_MAX)
    {
        addr = WRAM_ON_CHIP_ADDR_MIN + (addr % (32 * KiB));
    }

    addr = AlignAddress(addr, alignment);
    size_t index = addr - WRAM_ON_CHIP_ADDR_MIN;
    uint8_t* bytePtr = &(onChipWRAM_.at(index));
    uint32_t value = ReadPointer(bytePtr, alignment);
    return {value, 1};
}

int GameBoyAdvance::WriteOnChipWRAM(uint32_t addr, uint32_t value, AccessSize alignment)
{
    if (addr > WRAM_ON_CHIP_ADDR_MAX)
    {
        addr = WRAM_ON_CHIP_ADDR_MIN + (addr % (32 * KiB));
    }

    addr = AlignAddress(addr, alignment);
    size_t index = addr - WRAM_ON_CHIP_ADDR_MIN;
    uint8_t* bytePtr = &(onChipWRAM_.at(index));
    WritePointer(bytePtr, value, alignment);
    return 1;
}

//               Bus   Read      Write     Cycles
// I/O           32    8/16/32   8/16/32   1/1/1

std::pair<uint32_t, bool> GameBoyAdvance::ReadIoReg(uint32_t addr, AccessSize alignment)
{
    if (addr > IO_REG_ADDR_MAX)
    {
        addr = 0x0400'0800 + (addr % 4);
    }

    addr = AlignAddress(addr, alignment);

    // TODO: Placeholder until all I/O registers are properly handled
    size_t index = addr - IO_REG_ADDR_MIN;
    uint8_t* bytePtr = &(placeholderIoRegisters_.at(index));

    if (addr <= LCD_IO_ADDR_MAX)
    {
        return ppu_.ReadReg(addr, alignment);
    }
    else if (addr <= SOUND_IO_ADDR_MAX)
    {
        return apu_.ReadReg(addr, alignment);
    }
    else if (addr <= DMA_TRANSFER_CHANNELS_IO_ADDR_MAX)
    {
        return ReadDmaReg(addr, alignment);
    }
    else if (addr <= TIMER_IO_ADDR_MAX)
    {
        return {timerManager_.ReadReg(addr, alignment), false};
    }
    else if (addr <= SERIAL_COMMUNICATION_1_IO_ADDR_MAX)
    {
        return {ReadPointer(bytePtr, alignment), false};
    }
    else if (addr <= KEYPAD_INPUT_IO_ADDR_MAX)
    {
        return gamepad_.ReadReg(addr, alignment);
    }
    else if (addr <= SERIAL_COMMUNICATION_2_IO_ADDR_MAX)
    {
        return {ReadPointer(bytePtr, alignment), false};
    }
    else if (addr <= INT_WTST_PWRDWN_IO_ADDR_MAX)
    {
        if ((addr >= WAITCNT_ADDR) && (addr <= (WAITCNT_ADDR + 4)))
        {
            uint32_t value = gamePak_->ReadWAITCNT(addr, alignment);
            return {value, false};
        }

        return InterruptMgr.ReadReg(addr, alignment);
    }
    else
    {
        addr = INT_WTST_PWRDWN_IO_ADDR_MIN + ((addr % 0x0001'0000) - 0x0800);
        return InterruptMgr.ReadReg(addr, alignment);
    }
}

void GameBoyAdvance::WriteIoReg(uint32_t addr, uint32_t value, AccessSize alignment)
{
    if (addr > IO_REG_ADDR_MAX)
    {
        addr = 0x0400'0800 + (addr % 4);
    }

    addr = AlignAddress(addr, alignment);

    // TODO: Placeholder until all I/O registers are properly handled
    size_t index = addr - IO_REG_ADDR_MIN;
    uint8_t* bytePtr = &(placeholderIoRegisters_.at(index));

    if (addr <= LCD_IO_ADDR_MAX)
    {
        ppu_.WriteReg(addr, value, alignment);
    }
    else if (addr <= SOUND_IO_ADDR_MAX)
    {
        apu_.WriteReg(addr, value, alignment);
    }
    else if (addr <= DMA_TRANSFER_CHANNELS_IO_ADDR_MAX)
    {
        WriteDmaReg(addr, value, alignment);
    }
    else if (addr <= TIMER_IO_ADDR_MAX)
    {
        timerManager_.WriteReg(addr, value, alignment);
    }
    else if (addr <= SERIAL_COMMUNICATION_1_IO_ADDR_MAX)
    {
        WritePointer(bytePtr, value, alignment);
    }
    else if (addr <= KEYPAD_INPUT_IO_ADDR_MAX)
    {
        gamepad_.WriteReg(addr, value, alignment);
    }
    else if (addr <= SERIAL_COMMUNICATION_2_IO_ADDR_MAX)
    {
        WritePointer(bytePtr, value, alignment);
    }
    else if (addr <= INT_WTST_PWRDWN_IO_ADDR_MAX)
    {
        if ((addr <= WAITCNT_ADDR) && (WAITCNT_ADDR <= (addr + static_cast<uint8_t>(alignment) - 1)))
        {
            gamePak_->WriteWAITCNT(addr, value, alignment);
        }
        else
        {
            InterruptMgr.WriteReg(addr, value, alignment);
        }
    }
    else
    {
        addr = INT_WTST_PWRDWN_IO_ADDR_MIN + ((addr % 0x0001'0000) - 0x0800);
        InterruptMgr.WriteReg(addr, value, alignment);
    }
}

//                Bus   Read      Write     Cycles
//  Palette RAM   16    8/16/32   16/32     1/1/2 *
//  Plus 1 cycle if GBA accesses video memory at the same time. (TODO)

std::pair<uint32_t, int> GameBoyAdvance::ReadPaletteRAM(uint32_t addr, AccessSize alignment)
{
    if (addr > PALETTE_RAM_ADDR_MAX)
    {
        addr = PALETTE_RAM_ADDR_MIN + (addr % (1 * KiB));
    }

    int cycles = (alignment == AccessSize::WORD) ? 2 : 1;
    addr = AlignAddress(addr, alignment);
    size_t index = addr - PALETTE_RAM_ADDR_MIN;
    uint8_t* bytePtr = &(paletteRAM_.at(index));
    uint32_t value = ReadPointer(bytePtr, alignment);
    return {value, cycles};
}

int GameBoyAdvance::WritePaletteRAM(uint32_t addr, uint32_t value, AccessSize alignment)
{
    if (addr > PALETTE_RAM_ADDR_MAX)
    {
        addr = PALETTE_RAM_ADDR_MIN + (addr % (1 * KiB));
    }

    int cycles = (alignment == AccessSize::WORD) ? 2 : 1;

    if (alignment == AccessSize::BYTE)
    {
        alignment = AccessSize::HALFWORD;
        value = ((value & MAX_U8) << 8) | (value & MAX_U8);
    }

    addr = AlignAddress(addr, alignment);
    size_t index = addr - PALETTE_RAM_ADDR_MIN;
    uint8_t* bytePtr = &(paletteRAM_.at(index));
    WritePointer(bytePtr, value, alignment);
    return cycles;
}

//  Region        Bus   Read      Write     Cycles
//  VRAM          16    8/16/32   16/32     1/1/2 *
//  Plus 1 cycle if GBA accesses video memory at the same time. (TODO)

std::pair<uint32_t, int> GameBoyAdvance::ReadVRAM(uint32_t addr, AccessSize alignment)
{
    if (addr > VRAM_ADDR_MAX)
    {
        uint32_t adjustedAddr = VRAM_ADDR_MIN + (addr % (128 * KiB));

        if (adjustedAddr > VRAM_ADDR_MAX)
        {
            adjustedAddr -= (32 * KiB);
        }

        addr = adjustedAddr;
    }

    int cycles = (alignment == AccessSize::WORD) ? 2 : 1;
    addr = AlignAddress(addr, alignment);
    size_t index = addr - VRAM_ADDR_MIN;
    uint8_t* bytePtr = &(VRAM_.at(index));
    uint32_t value = ReadPointer(bytePtr, alignment);
    return {value, cycles};
}

int GameBoyAdvance::WriteVRAM(uint32_t addr, uint32_t value, AccessSize alignment)
{
    if (addr > VRAM_ADDR_MAX)
    {
        uint32_t adjustedAddr = VRAM_ADDR_MIN + (addr % (128 * KiB));

        if (adjustedAddr > VRAM_ADDR_MAX)
        {
            adjustedAddr -= (32 * KiB);
        }

        addr = adjustedAddr;
    }

    if (alignment == AccessSize::BYTE)
    {
        if (((ppu_.BgMode() <= 2) && (addr >= 0x0601'0000)) || ((ppu_.BgMode() > 2) && (addr >= 0x0601'4000)))
        {
            return 1;
        }

        alignment = AccessSize::HALFWORD;
        value = ((value & MAX_U8) << 8) | (value & MAX_U8);
    }

    int cycles = (alignment == AccessSize::WORD) ? 2 : 1;
    addr = AlignAddress(addr, alignment);
    size_t index = addr - VRAM_ADDR_MIN;
    uint8_t* bytePtr = &(VRAM_.at(index));
    WritePointer(bytePtr, value, alignment);
    return cycles;
}

//  Region        Bus   Read      Write     Cycles
//  OAM           32    8/16/32   16/32     1/1/1 *
//  Plus 1 cycle if GBA accesses video memory at the same time. (TODO)

std::pair<uint32_t, int> GameBoyAdvance::ReadOAM(uint32_t addr, AccessSize alignment)
{
    if (addr > OAM_ADDR_MAX)
    {
        addr = OAM_ADDR_MIN + (addr % (1 * KiB));
    }

    addr = AlignAddress(addr, alignment);
    size_t index = addr - OAM_ADDR_MIN;
    uint8_t* bytePtr = &(OAM_.at(index));
    uint32_t value = ReadPointer(bytePtr, alignment);
    return {value, 1};
}

int GameBoyAdvance::WriteOAM(uint32_t addr, uint32_t value, AccessSize alignment)
{
    if (alignment == AccessSize::BYTE)
    {
        return 1;
    }

    if (addr > OAM_ADDR_MAX)
    {
        addr = OAM_ADDR_MIN + (addr % (1 * KiB));
    }

    addr = AlignAddress(addr, alignment);
    size_t index = addr - OAM_ADDR_MIN;
    uint8_t* bytePtr = &(OAM_.at(index));
    WritePointer(bytePtr, value, alignment);
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
