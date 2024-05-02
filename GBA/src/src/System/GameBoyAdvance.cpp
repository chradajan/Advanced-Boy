#include <System/GameBoyAdvance.hpp>
#include <array>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <Config.hpp>
#include <Logging/Logging.hpp>
#include <System/MemoryMap.hpp>
#include <System/InterruptManager.hpp>
#include <System/Scheduler.hpp>
#include <System/Utilities.hpp>
#include <Timers/TimerManager.hpp>

namespace fs = std::filesystem;

GameBoyAdvance::GameBoyAdvance(fs::path const biosPath, std::function<void(int)> refreshScreenCallback) :
    biosLoaded_(LoadBIOS(biosPath)),
    cpu_(this, biosLoaded_),
    ppu_(paletteRAM_, VRAM_, OAM_)
{
    Scheduler.RegisterEvent(EventType::RefreshScreen, refreshScreenCallback);
    Scheduler.RegisterEvent(EventType::Halt, std::bind(&Halt, this, std::placeholders::_1));
    gamePakLoaded_ = false;
    halted_ = false;
    mirroredIoReg_ = 0;
    lastBiosFetch_ = 0;
}

std::pair<uint32_t, int> GameBoyAdvance::ReadMemory(uint32_t addr, AccessSize alignment)
{
    uint8_t page = (addr & 0x0F00'0000) >> 24;
    uint32_t value;
    int cycles;
    bool openBus = false;

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
            if ((addr <= IO_REG_ADDR_MAX) || (((addr % 0x0001'0000) - 0x0800) < 4))
            {
                std::tie(value, cycles, openBus) = ReadIoReg(addr, alignment);
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
        case 0x08 ... 0x0D:  // GamePak ROM/FlashROM
        {
            if (addr <= GAME_PAK_ROM_ADDR_MAX)
            {
                std::tie(value, cycles, openBus) = gamePak_->ReadROM(addr, alignment);
            }
            else
            {
                openBus = true;
            }

            break;
        }
        case 0x0E ... 0x0F:  // GamePak SRAM
            std::tie(value, cycles) = gamePak_->ReadSRAM(addr, alignment);
            break;
        default:
            openBus = true;
            break;
    }

    if (openBus)
    {
        std::tie(value, cycles) = ReadOpenBus(addr, alignment);
    }

    return {value, cycles};
}

int GameBoyAdvance::WriteMemory(uint32_t addr, uint32_t value, AccessSize alignment)
{
    uint8_t page = (addr & 0x0F00'0000) >> 24;
    int cycles = 1;

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
            if ((addr <= IO_REG_ADDR_MAX) || (((addr % 0x0001'0000) - 0x0800) < 4))
            {
                cycles = WriteIoReg(addr, value, alignment);
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
        case 0x08 ... 0x0D:  // GamePak ROM/FlashROM
            break;
        case 0x0E ... 0x0F:  // GamePak SRAM
            cycles = gamePak_->WriteSRAM(addr, value, alignment);
            break;
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

std::string GameBoyAdvance::RomTitle() const
{
    if (gamePakLoaded_)
    {
        return gamePak_->RomTitle();
    }

    return "";
}

void GameBoyAdvance::Run()
{
    if (!biosLoaded_ && !gamePakLoaded_)
    {
        return;
    }

    try
    {
        while (true)
        {
            if (halted_)
            {
                Scheduler.SkipToNextEvent();
            }
            else
            {
                int cpuCycles = cpu_.Tick();
                Scheduler.Tick(cpuCycles);
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
//  **  Default waitstate settings (TODO)

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
    return {value, 1};
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
    return 1;
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

std::tuple<uint32_t, int, bool> GameBoyAdvance::ReadIoReg(uint32_t addr, AccessSize alignment)
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
        return ppu_.ReadLcdReg(addr, alignment);
    }
    else if (addr <= SOUND_IO_ADDR_MAX)
    {
        return {ReadPointer(bytePtr, alignment), 1, false};
    }
    else if (addr <= DMA_TRANSFER_CHANNELS_IO_ADDR_MAX)
    {
        return {ReadPointer(bytePtr, alignment), 1, false};
    }
    else if (addr <= TIMER_IO_ADDR_MAX)
    {
        return {timers_.ReadIoReg(addr, alignment), 1, false};
    }
    else if (addr <= SERIAL_COMMUNICATION_1_IO_ADDR_MAX)
    {
        return {ReadPointer(bytePtr, alignment), 1, false};
    }
    else if (addr <= KEYPAD_INPUT_IO_ADDR_MAX)
    {
        return gamepad_.ReadGamepadReg(addr, alignment);
    }
    else if (addr <= SERIAL_COMMUNICATION_2_IO_ADDR_MAX)
    {
        return {ReadPointer(bytePtr, alignment), 1, false};
    }
    else if (addr <= INT_WTST_PWRDWN_IO_ADDR_MAX)
    {
        if ((addr >= WAITCNT_ADDR) && (addr <= (WAITCNT_ADDR + 4)))
        {
            auto [value, cycles] = gamePak_->ReadWAITCNT(addr, alignment);
            return {value, cycles, false};
        }

        return InterruptMgr.ReadIoReg(addr, alignment);
    }
    else
    {
        addr = INT_WTST_PWRDWN_IO_ADDR_MIN + ((addr % 0x0001'0000) - 0x0800);
        return InterruptMgr.ReadIoReg(addr, alignment);
    }
}

int GameBoyAdvance::WriteIoReg(uint32_t addr, uint32_t value, AccessSize alignment)
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
        return ppu_.WriteLcdReg(addr, value, alignment);
    }
    else if (addr <= SOUND_IO_ADDR_MAX)
    {
        WritePointer(bytePtr, value, alignment);
        return 1;
    }
    else if (addr <= DMA_TRANSFER_CHANNELS_IO_ADDR_MAX)
    {
        WritePointer(bytePtr, value, alignment);
        return 1;
    }
    else if (addr <= TIMER_IO_ADDR_MAX)
    {
        timers_.WriteIoReg(addr, value, alignment);
        return 1;
    }
    else if (addr <= SERIAL_COMMUNICATION_1_IO_ADDR_MAX)
    {
        WritePointer(bytePtr, value, alignment);
        return 1;
    }
    else if (addr <= KEYPAD_INPUT_IO_ADDR_MAX)
    {
        return gamepad_.WriteGamepadReg(addr, value, alignment);
    }
    else if (addr <= SERIAL_COMMUNICATION_2_IO_ADDR_MAX)
    {
        WritePointer(bytePtr, value, alignment);
        return 1;
    }
    else if (addr <= INT_WTST_PWRDWN_IO_ADDR_MAX)
    {
        if ((addr <= WAITCNT_ADDR) && (WAITCNT_ADDR <= (addr + static_cast<uint8_t>(alignment) - 1)))
        {
            return gamePak_->WriteWAITCNT(addr, value, alignment);
        }

        return InterruptMgr.WriteIoReg(addr, value, alignment);
    }
    else
    {
        addr = INT_WTST_PWRDWN_IO_ADDR_MIN + ((addr % 0x0001'0000) - 0x0800);
        return InterruptMgr.WriteIoReg(addr, value, alignment);
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

// Open Bus (TODO)

std::pair<uint32_t, int> GameBoyAdvance::ReadOpenBus(uint32_t addr, AccessSize alignment)
{
    (void)addr; (void)alignment;
    return {0, 1};

    throw std::runtime_error(
        std::format("Open bus not implemented. Addr: {:08X}, Access Size: {}", addr, static_cast<uint8_t>(alignment)));
}
