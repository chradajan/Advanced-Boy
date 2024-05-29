#include <Cartridge/SRAM.hpp>
#include <array>
#include <bit>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <utility>
#include <System/MemoryMap.hpp>
#include <System/SystemControl.hpp>
#include <Utilities/MemoryUtilities.hpp>

namespace Cartridge
{
SRAM::SRAM(fs::path savePath) :
    savePath_(savePath)
{
    if (fs::exists(savePath) && (fs::file_size(savePath) == sram_.size()))
    {
        std::ifstream saveFile(savePath, std::ios::binary);

        if (!saveFile.fail())
        {
            saveFile.read(reinterpret_cast<char*>(sram_.data()), sram_.size());
        }
    }
    else
    {
        sram_.fill(0);
    }
}

SRAM::~SRAM()
{
    std::ofstream saveFile(savePath_, std::ios::binary);

    if (!saveFile.fail())
    {
        saveFile.write(reinterpret_cast<char*>(sram_.data()), sram_.size());
    }
}

std::pair<uint32_t, int> SRAM::Read(uint32_t addr, AccessSize alignment)
{
    int cycles = 1;
    cycles += SystemController.WaitStates(WaitState::SRAM, false, alignment);
    size_t index = (addr - SRAM_ADDR_MIN) % sram_.size();
    uint32_t value = sram_[index];

    if (alignment != AccessSize::BYTE)
    {
        value = Read8BitBus(value, alignment);
    }

    return {value, cycles};
}

int SRAM::Write(uint32_t addr, uint32_t value, AccessSize alignment)
{
    int cycles = 1;
    cycles += SystemController.WaitStates(WaitState::SRAM, false, alignment);

    if (alignment != AccessSize::BYTE)
    {
        value = Write8BitBus(addr, value);
    }

    size_t index = (addr - SRAM_ADDR_MIN) % sram_.size();
    sram_[index] = value;
    return cycles;
}
}
