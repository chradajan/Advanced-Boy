#include <Cartridge/Flash.hpp>
#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <utility>
#include <vector>
#include <System/MemoryMap.hpp>
#include <System/SystemControl.hpp>
#include <Utilities/MemoryUtilities.hpp>

namespace Cartridge
{
Flash::Flash(fs::path savePath, size_t flashSizeInBytes) :
    savePath_(savePath)
{
    if (flashSizeInBytes == FLASH_BANK_SIZE)
    {
        flash_.resize(1);
    }
    else if (flashSizeInBytes == (2 * FLASH_BANK_SIZE))
    {
        flash_.resize(2);
    }

    for (auto& bank : flash_)
    {
        bank.fill(0xFF);
    }

    if (fs::exists(savePath) && (fs::file_size(savePath) == flashSizeInBytes))
    {
        std::ifstream saveFile(savePath, std::ios::binary);

        if (!saveFile.fail())
        {
            for (auto& bank : flash_)
            {
                saveFile.read(reinterpret_cast<char*>(bank.data()), bank.size());
            }
        }
    }
}

Flash::~Flash()
{
    if (!flash_.empty())
    {
        std::ofstream saveFile(savePath_, std::ios::binary);

        if (!saveFile.fail())
        {
            for (auto& bank : flash_)
            {
                saveFile.write(reinterpret_cast<char*>(bank.data()), bank.size());
            }
        }
    }
}

void Flash::Reset()
{
    state_ = FlashState::READY;
    chipIdMode_ = false;
    eraseMode_ = false;
    bank_ = 0;
}

std::pair<uint32_t, int> Flash::Read(uint32_t addr, AccessSize alignment)
{
    if (addr > 0x0E00'FFFF)
    {
        addr = ((addr - SRAM_ADDR_MIN) % FLASH_BANK_SIZE) + SRAM_ADDR_MIN;
    }

    uint32_t value = 0;
    int cycles = 1;
    cycles += SystemController.WaitStates(WaitState::SRAM, false, alignment);

    if (chipIdMode_ && (addr == 0x0E00'0000))
    {
        value = (flash_.size() == 2) ? 0x62 : 0x32;
    }
    else if (chipIdMode_ && (addr = 0x0E00'0001))
    {
        value = (flash_.size() == 2) ? 0x13 : 0x1B;
    }
    else
    {
        size_t index = addr - SRAM_ADDR_MIN;
        value = flash_.at(bank_).at(index);
    }

    if (alignment != AccessSize::BYTE)
    {
        value = Read8BitBus(value, alignment);
    }

    return {value, cycles};
}

int Flash::Write(uint32_t addr, uint32_t value, AccessSize alignment)
{
    uint8_t byte = value & MAX_U8;
    int cycles = 1;
    cycles += SystemController.WaitStates(WaitState::SRAM, false, alignment);

    if (alignment != AccessSize::BYTE)
    {
        byte = Write8BitBus(addr, value);
        alignment = AccessSize::BYTE;
    }

    auto cmd = FlashCommand{byte};

    if (addr > 0x0E00'FFFF)
    {
        addr = ((addr - SRAM_ADDR_MIN) % FLASH_BANK_SIZE) + SRAM_ADDR_MIN;
    }

    switch (state_)
    {
        case FlashState::READY:
        {
            if ((addr == 0x0E00'5555) && (cmd == FlashCommand::START_CMD_SEQ))
            {
                state_ = FlashState::CMD_SEQ_STARTED;
            }

            break;
        }
        case FlashState::CMD_SEQ_STARTED:
        {
            if ((addr == 0x0E00'2AAA) && (cmd == FlashCommand::AWAIT_CMD))
            {
                state_ = FlashState::AWAITING_CMD;
            }

            break;
        }
        case FlashState::AWAITING_CMD:
        {
            if (addr != 0x0E00'5555)
            {
                break;
            }

            switch (cmd)
            {
                case FlashCommand::ENTER_CHIP_ID_MODE:
                    chipIdMode_ = true;
                    state_ = FlashState::READY;
                    break;
                case FlashCommand::EXIT_CHIP_ID_MODE:
                    chipIdMode_ = false;
                    state_ = FlashState::READY;
                    break;
                case FlashCommand::PREPARE_TO_RCV_ERASE_CMD:
                    state_ = FlashState::ERASE_SEQ_READY;
                    break;
                case FlashCommand::PREPARE_TO_WRITE_BYTE:
                    state_ = FlashState::PREPARE_TO_WRITE;
                    break;
                case FlashCommand::SET_MEMORY_BANK:
                    state_ = FlashState::AWAITING_MEMORY_BANK;
                    break;
                default:
                    break;
            }

            break;
        }
        case FlashState::ERASE_SEQ_READY:
        {
            if ((addr == 0x0E00'5555) && (cmd == FlashCommand::START_CMD_SEQ))
            {
                state_ = FlashState::ERASE_SEQ_STARTED;
            }

            break;
        }
        case FlashState::ERASE_SEQ_STARTED:
        {
            if ((addr == 0x0E00'2AAA) && (cmd == FlashCommand::AWAIT_CMD))
            {
                state_ = FlashState::AWAITING_ERASE_CMD;
            }

            break;
        }
        case FlashState::AWAITING_ERASE_CMD:
        {
            switch (cmd)
            {
                case FlashCommand::ERASE_ENTIRE_CHIP:
                {
                    if (addr == 0x0E00'5555)
                    {
                        for (auto& bank : flash_)
                        {
                            std::fill(bank.begin(), bank.end(), 0xFF);
                        }

                        state_ = FlashState::READY;
                    }

                    break;
                }
                case FlashCommand::ERASE_4K_SECTOR:
                {
                    size_t block = addr & 0x0000'F000;
                    auto blockStart = flash_.at(bank_).begin() + block;
                    auto blockEnd = flash_.at(bank_).begin() + block + 0x1000;
                    std::fill(blockStart, blockEnd, 0xFF);
                    state_ = FlashState::READY;
                    break;
                }
                default:
                    break;
            }

            break;
        }
        case FlashState::PREPARE_TO_WRITE:
        {
            size_t index = addr - SRAM_ADDR_MIN;
            flash_.at(bank_).at(index) = byte;
            state_ = FlashState::READY;
            break;
        }
        case FlashState::AWAITING_MEMORY_BANK:
        {
            if (addr != 0x0E00'0000)
            {
                break;
            }

            bank_ = byte & 0x01;
            state_ = FlashState::READY;
            break;
        }
    }

    return cycles;
}
}
