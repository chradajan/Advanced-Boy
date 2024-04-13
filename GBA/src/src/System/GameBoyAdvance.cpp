#include <System/GameBoyAdvance.hpp>
#include <Config.hpp>
#include <Logging/Logging.hpp>
#include <MemoryMap.hpp>
#include <System/InterruptManager.hpp>
#include <System/Scheduler.hpp>
#include <array>
#include <filesystem>
#include <format>
#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>

namespace fs = std::filesystem;

GameBoyAdvance::GameBoyAdvance(fs::path const biosPath, std::function<void(int)> refreshScreenCallback) :
    cpu_(std::bind(&ReadMemory,  this, std::placeholders::_1, std::placeholders::_2),
         std::bind(&WriteMemory, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)),
    ppu_(paletteRAM_, VRAM_, OAM_),
    biosLoaded_(biosPath != ""),
    gamePakLoaded_(false),
    ppuCatchupCycles_(0)
{
    Scheduler.RegisterEvent(EventType::REFRESH_SCREEN, refreshScreenCallback);
}

bool GameBoyAdvance::LoadGamePak(fs::path romPath)
{
    gamePak_.reset();
    gamePak_ = std::make_unique<Cartridge::GamePak>(romPath);
    ZeroMemory();
    gamePakLoaded_ = gamePak_->RomLoaded();
    return gamePakLoaded_;
}

void GameBoyAdvance::Run()
{
    if (!biosLoaded_ && !gamePakLoaded_)
    {
        return;
    }

    while (true)
    {
        try
        {
            int cpuCycles = cpu_.Tick();
            Scheduler.Tick(cpuCycles);
        }
        catch (std::runtime_error const& error)
        {
            Logging::LogMgr.LogException(error);
            Logging::LogMgr.DumpLogs();
            throw;
        }
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

std::pair<uint8_t*, int> GameBoyAdvance::GetPointerToMem(uint32_t addr, uint8_t accessSize)
{
    uint8_t* mappedPtr = nullptr;
    size_t adjustedIndex = 0;
    size_t maxIndex = 0;

    // TODO Implement actual access times
    int cycles = 1;

    if (addr <= BIOS_ADDR_MAX)
    {
        adjustedIndex = addr;
        maxIndex = BIOS_ADDR_MAX;
        mappedPtr = &BIOS_.at(adjustedIndex);
    }
    else if ((addr >= WRAM_ON_BOARD_ADDR_MIN) && (addr <= WRAM_ON_BOARD_ADDR_MAX))
    {
        adjustedIndex = addr - WRAM_ON_BOARD_ADDR_MIN;
        maxIndex = WRAM_ON_BOARD_ADDR_MAX;
        mappedPtr = &onBoardWRAM_.at(adjustedIndex);
    }
    else if ((addr >= WRAM_ON_CHIP_ADDR_MIN) && (addr <= WRAM_ON_CHIP_ADDR_MAX))
    {
        adjustedIndex = addr - WRAM_ON_CHIP_ADDR_MIN;
        maxIndex = WRAM_ON_CHIP_ADDR_MAX;
        mappedPtr = &onChipWRAM_.at(adjustedIndex);
    }
    else if ((addr >= PALETTE_RAM_ADDR_MIN) && (addr <= PALETTE_RAM_ADDR_MAX))
    {
        adjustedIndex = addr - PALETTE_RAM_ADDR_MIN;
        maxIndex = PALETTE_RAM_ADDR_MAX;
        mappedPtr = &paletteRAM_.at(adjustedIndex);
    }
    else if ((addr >= VRAM_ADDR_MIN) && (addr <= VRAM_ADDR_MAX))
    {
        adjustedIndex = addr - VRAM_ADDR_MIN;
        maxIndex = VRAM_ADDR_MAX;
        mappedPtr = &VRAM_.at(adjustedIndex);
    }
    else if ((addr >= OAM_ADDR_MIN) && (addr <= OAM_ADDR_MAX))
    {
        adjustedIndex = addr - OAM_ADDR_MIN;
        maxIndex = OAM_ADDR_MAX;
        mappedPtr = &OAM_.at(adjustedIndex);
    }

    if ((adjustedIndex + accessSize - 1) > maxIndex)
    {
        throw std::runtime_error(std::format("Illegal memory access. Addr: 0x{:08X}, Size: {}", addr, accessSize));
    }
    else if (!mappedPtr)
    {
        throw std::runtime_error(std::format("Illegal memory access. Could not map address: 0x{:08X}, Size: {}", addr, accessSize));
    }

    return {mappedPtr, cycles};
}

std::pair<uint32_t, int> GameBoyAdvance::ReadMemory(uint32_t addr, int accessSize)
{
    if ((addr >= IO_REG_ADDR_MIN) && (addr <= IO_REG_ADDR_MAX))
    {
        return ReadIoReg(addr, accessSize);
    }
    else if ((addr >= GAME_PAK_ROM_ADDR_MIN) && (addr <= GAME_PAK_SRAM_ADDR_MAX))
    {
        return gamePak_->ReadMemory(addr, accessSize);
    }
    else
    {
        auto [bytePtr, cycles] = GetPointerToMem(addr, accessSize);
        uint32_t value;

        switch (accessSize)
        {
            case 1:
                value = *bytePtr;
                break;
            case 2:
                value = *reinterpret_cast<uint16_t*>(bytePtr);
                break;
            case 4:
                value = *reinterpret_cast<uint32_t*>(bytePtr);
                break;
            default:
                throw std::runtime_error("Illegal Read Memory access size");
        }

        return {value, cycles};
    }
}

int GameBoyAdvance::WriteMemory(uint32_t addr, uint32_t value, int accessSize)
{
    if ((addr >= IO_REG_ADDR_MIN) && (addr <= IO_REG_ADDR_MAX))
    {
        return WriteIoReg(addr, value, accessSize);
    }
    else if ((addr >= GAME_PAK_ROM_ADDR_MIN) && (addr <= GAME_PAK_SRAM_ADDR_MAX))
    {
        return gamePak_->WriteMemory(addr, value, accessSize);
    }
    else
    {
        auto [bytePtr, cycles] = GetPointerToMem(addr, accessSize);

        switch (accessSize)
        {
            case 1:
                *bytePtr = value;
                break;
            case 2:
                *reinterpret_cast<uint16_t*>(bytePtr) = value;
                break;
            case 4:
                *reinterpret_cast<uint32_t*>(bytePtr) = value;
                break;
            default:
                throw std::runtime_error("Illegal Write Memory access size");
        }

        return cycles;
    }
}

std::pair<uint32_t, int> GameBoyAdvance::ReadIoReg(uint32_t addr, int accessSize)
{
    if ((addr >= LCD_IO_ADDR_MIN) && (addr <= LCD_IO_ADDR_MAX))
    {
        return ppu_.ReadLcdReg(addr, accessSize);
    }
    else if (addr == WAITCNT_ADDR)
    {
        return gamePak_->ReadWAITCNT();
    }
    else if ((addr >= INT_WTST_PWRDWN_IO_ADDR_MIN) && (addr <= INT_WTST_PWRDWN_IO_ADDR_MAX))
    {
        return InterruptMgr.ReadIoReg(addr, accessSize);
    }
    else
    {
        // TODO Implement memory mirroring for 4000800h every 64K.
        uint8_t* bytePtr = &placeholderIoRegisters_[addr - IO_REG_ADDR_MIN];

        switch (accessSize)
        {
            case 1:
                return {*bytePtr, 1};
            case 2:
                return {*reinterpret_cast<uint16_t*>(bytePtr), 1};
            case 4:
                return {*reinterpret_cast<uint32_t*>(bytePtr), 1};
            default:
                throw std::runtime_error("Illegal Read Memory access size");
        }
    }
}

int GameBoyAdvance::WriteIoReg(uint32_t addr, uint32_t value, int accessSize)
{
    if ((addr >= LCD_IO_ADDR_MIN) && (addr <= LCD_IO_ADDR_MAX))
    {
        return ppu_.WriteLcdReg(addr, value, accessSize);
    }
    else if (addr == WAITCNT_ADDR)
    {
        return gamePak_->WriteWAITCNT(value);
    }
    else if ((addr >= INT_WTST_PWRDWN_IO_ADDR_MIN) && (addr <= INT_WTST_PWRDWN_IO_ADDR_MAX))
    {
        return InterruptMgr.WriteIoReg(addr, value, accessSize);
    }
    else
    {
        // TODO Implement memory mirroring for 4000800h every 64K.
        uint8_t* bytePtr = &placeholderIoRegisters_[addr - IO_REG_ADDR_MIN];

        switch (accessSize)
        {
            case 1:
                *bytePtr = value;
                break;
            case 2:
                *reinterpret_cast<uint16_t*>(bytePtr) = value;
                break;
            case 4:
                *reinterpret_cast<uint32_t*>(bytePtr) = value;
                break;
            default:
                throw std::runtime_error("Illegal Read Memory access size");
        }

        return 1;
    }
}
