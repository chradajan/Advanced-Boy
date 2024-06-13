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
#include <Audio/APU.hpp>
#include <Cartridge/GamePak.hpp>
#include <DMA/DmaChannel.hpp>
#include <DMA/DmaManager.hpp>
#include <Gamepad/GamepadManager.hpp>
#include <Graphics/PPU.hpp>
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
    dmaMgr_(*this),
    gamePak_(nullptr)
{
    // State
    gamePakLoaded_ = false;

    Scheduler.RegisterEvent(EventType::HBlank, std::bind(&HBlank, this, std::placeholders::_1));
    Scheduler.RegisterEvent(EventType::VBlank, std::bind(&VBlank, this, std::placeholders::_1));
    Scheduler.RegisterEvent(EventType::Timer0Overflow, std::bind(&Timer0Overflow, this, std::placeholders::_1));
    Scheduler.RegisterEvent(EventType::Timer1Overflow, std::bind(&Timer1Overflow, this, std::placeholders::_1));
}

GameBoyAdvance::~GameBoyAdvance()
{
    Logging::LogMgr.DumpLogs();
}

void GameBoyAdvance::Reset()
{
    // Scheduler
    Scheduler.Reset();

    // Components
    apu_.Reset();
    cpu_.Reset();
    dmaMgr_.Reset();
    gamepad_.Reset();
    ppu_.Reset();
    timerMgr_.Reset();
    SystemController.Reset();

    if (gamePakLoaded_)
    {
        gamePak_->Reset();
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

void GameBoyAdvance::FillAudioBuffer()
{
    if (!biosLoaded_ && !gamePakLoaded_)
    {
        return;
    }

    size_t samplesToGenerate = apu_.FreeBufferSpace();

    while (samplesToGenerate > 0)
    {
        Run(samplesToGenerate);
        samplesToGenerate = apu_.FreeBufferSpace();
    }
}

void GameBoyAdvance::Run(size_t samples)
{
    apu_.ClearSampleCounter();

    while (apu_.GetSampleCounter() < samples)
    {
        if (SystemController.Halted() || dmaMgr_.DmaActive())
        {
            Scheduler.SkipToNextEvent();
        }
        else
        {
            cpu_.Step(Scheduler.GetPendingIRQ());
            Scheduler.CheckEventQueue();
        }
    }
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
        dmaMgr_.CheckHBlankChannels();
    }
}

void GameBoyAdvance::VBlank(int extraCycles)
{
    ppu_.VBlank(extraCycles);

    if (ppu_.CurrentScanline() == 160)
    {
        dmaMgr_.CheckVBlankChannels();
    }
}

void GameBoyAdvance::TimerOverflow(int timer, int extraCycles)
{
    switch (timer)
    {
        case 0:
            timerMgr_.Timer0Overflow(extraCycles);
            break;
        case 1:
            timerMgr_.Timer1Overflow(extraCycles);
            break;
        default:
            return;
    }

    auto [replenishA, replenishB] = apu_.TimerOverflow(timer);

    if (replenishA)
    {
        dmaMgr_.CheckFifoAChannels();
    }

    if (replenishB)
    {
        dmaMgr_.CheckFifoBChannels();
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
            std::tie(value, openBus) = dmaMgr_.ReadReg(addr, alignment);
            break;
        case TIMER_IO_ADDR_MIN ... TIMER_IO_ADDR_MAX:
            std::tie(value, openBus) = timerMgr_.ReadReg(addr, alignment);
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
            dmaMgr_.WriteReg(addr, value, alignment);
            break;
        case TIMER_IO_ADDR_MIN ... TIMER_IO_ADDR_MAX:
            timerMgr_.WriteReg(addr, value, alignment);
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
