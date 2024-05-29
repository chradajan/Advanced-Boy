#include <Cartridge/EEPROM.hpp>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <utility>
#include <vector>
#include <System/SystemControl.hpp>
#include <Utilities/MemoryUtilities.hpp>

namespace Cartridge
{
EEPROM::EEPROM(fs::path savePath) :
    savePath_(savePath)
{
    if (fs::exists(savePath))
    {
        size_t sizeInBytes = fs::file_size(savePath);

        if ((sizeInBytes == 512) || (sizeInBytes == 8 * KiB))
        {
            eeprom_.resize(sizeInBytes / sizeof(uint64_t));
            std::ifstream saveFile(savePath, std::ios::binary);

            if (!saveFile.fail())
            {
                saveFile.read(reinterpret_cast<char*>(eeprom_.data()), sizeInBytes);
            }
        }
    }

    currentIndex_ = MAX_U16;
}

EEPROM::~EEPROM()
{
    if (!eeprom_.empty())
    {
        std::ofstream saveFile(savePath_, std::ios::binary);

        if (!saveFile.fail())
        {
            saveFile.write(reinterpret_cast<char*>(eeprom_.data()), eeprom_.size() * sizeof(eeprom_[0]));
        }
    }
}

void EEPROM::Reset()
{
    currentIndex_ = MAX_U16;
}

std::pair<uint32_t, int> EEPROM::Read(uint32_t addr, AccessSize alignment)
{
    (void)addr;
    int cycles = 1;
    cycles += SystemController.WaitStates(WaitState::TWO, false, alignment);
    return {1, cycles};
}

int EEPROM::Write(uint32_t addr, uint32_t value, AccessSize alignment)
{
    (void)addr;
    (void)value;
    int cycles = 1;
    cycles += SystemController.WaitStates(WaitState::TWO, false, alignment);
    return cycles;
}

int EEPROM::SetIndex(size_t index, size_t indexLength)
{
    int cycles = indexLength + 3;
    cycles += SystemController.WaitStates(WaitState::TWO, false, AccessSize::HALFWORD) +
              (SystemController.WaitStates(WaitState::TWO, true, AccessSize::HALFWORD) * (indexLength + 2));

    if (eeprom_.empty())
    {
        if (indexLength == 6)
        {
            eeprom_.resize(64, MAX_U64);
        }
        else if (indexLength == 14)
        {
            eeprom_.resize(1024, MAX_U64);
        }
    }

    currentIndex_ = index & 0x03FF;
    return cycles;
}

std::pair<uint64_t, int> EEPROM::ReadDoubleWord()
{
    int cycles = 68;
    cycles += SystemController.WaitStates(WaitState::TWO, false, AccessSize::HALFWORD) +
              (SystemController.WaitStates(WaitState::TWO, true, AccessSize::HALFWORD) * 67);

    if (eeprom_.empty() || (currentIndex_ >= eeprom_.size()))
    {
        return {MAX_U64, cycles};
    }

    return {eeprom_[currentIndex_], cycles};
}

int EEPROM::WriteDoubleWord(size_t index, size_t indexLength, uint64_t value)
{
    int cycles = 67 + indexLength;
    cycles += SystemController.WaitStates(WaitState::TWO, false, AccessSize::HALFWORD) +
              (SystemController.WaitStates(WaitState::TWO, true, AccessSize::HALFWORD) * (indexLength + 66));

    if (eeprom_.empty())
    {
        if (indexLength == 6)
        {
            eeprom_.resize(64, MAX_U64);
        }
        else if (indexLength == 14)
        {
            eeprom_.resize(1024, MAX_U64);
        }
    }

    index &= 0x03FF;

    if (!eeprom_.empty() && (index < eeprom_.size()))
    {
        eeprom_[index] = value;
    }

    return cycles;
}
}
