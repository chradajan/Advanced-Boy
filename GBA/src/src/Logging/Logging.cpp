#include <Logging/Logging.hpp>
#include <Config.hpp>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <stdexcept>
#include <string>
#include <System/InterruptManager.hpp>
#include <System/Scheduler.hpp>

namespace fs = std::filesystem;

namespace
{
std::string InterruptString(InterruptType interrupt)
{
    switch (interrupt)
    {
        case InterruptType::LCD_VBLANK:
            return "LCD_VBLANK";
        case InterruptType::LCD_HBLANK:
            return "LCD_HBLANK";
        case InterruptType::LCD_VCOUNTER_MATCH:
            return "LCD_VCOUNTER_MATCH";
        case InterruptType::TIMER_0_OVERFLOW:
            return "TIMER_0_OVERFLOW";
        case InterruptType::TIMER_1_OVERFLOW:
            return "TIMER_1_OVERFLOW";
        case InterruptType::TIMER_2_OVERFLOW:
            return "TIMER_2_OVERFLOW";
        case InterruptType::TIMER_3_OVERFLOW:
            return "TIMER_3_OVERFLOW";
        case InterruptType::SERIAL_COMMUNICATION:
            return "SERIAL_COMMUNICATION";
        case InterruptType::DMA0:
            return "DMA0";
        case InterruptType::DMA1:
            return "DMA1";
        case InterruptType::DMA2:
            return "DMA2";
        case InterruptType::DMA3:
            return "DMA3";
        case InterruptType::KEYPAD:
            return "KEYPAD";
        case InterruptType::GAME_PAK:
            return "GAME_PAK";
    }

    return "";
}
}

namespace Logging
{
LogManager::LogManager() :
    loggingInitialized_(false)
{
}

void LogManager::Initialize()
{
    logPath_ = LOG_PATH;

    if (!logPath_.empty() && Config::LOGGING_ENABLED && !loggingInitialized_)
    {
        if (!fs::exists(logPath_))
        {
            fs::create_directory(logPath_);
        }

        logPath_ /= "log.log";

        if (fs::exists(logPath_))
        {
            fs::remove(logPath_);
        }

        loggingInitialized_ = true;
    }
}

void LogManager::LogInstruction(uint32_t pc, std::string mnemonic, std::string registers)
{
    LogMessage(std::format("{:08X}:  {:<40}  {}", pc, mnemonic, registers));
}

void LogManager::LogIRQ()
{
    LogMessage("Servicing IRQ");
}

void LogManager::LogHalt(uint16_t IE)
{
    std::string enabledInterrupts = "";

    for (int i = 0; i < 14; ++i)
    {
        uint16_t mask = (0x0001 << i);

        if (IE & mask)
        {
            if (enabledInterrupts != "")
            {
                enabledInterrupts += " | " + InterruptString(static_cast<InterruptType>(mask));
            }
            else
            {
                enabledInterrupts += InterruptString(static_cast<InterruptType>(mask));
            }
        }
    }

    LogMessage("Halting - IE: " + enabledInterrupts);
}

void LogManager::LogUnhalt(uint16_t IF, uint16_t IE)
{
    std::string unhaltReason = InterruptString(static_cast<InterruptType>(IF & IE));
    LogMessage("Unhalting due to " + unhaltReason);
}

void LogManager::LogInterruptRequest(InterruptType interrupt)
{
    LogMessage("Requesting " + InterruptString(interrupt) + " interrupt");
}

void LogManager::LogException(std::exception const& error)
{
    LogMessage(error.what());
}

void LogManager::DumpLogs()
{
    if (loggingInitialized_)
    {
        std::ofstream logFile;
        logFile.open(logPath_);

        while (!buffer_.Empty())
        {
            logFile << buffer_.Pop();
        }
    }
}

void LogManager::LogMessage(std::string message)
{
    if (loggingInitialized_)
    {
        if (buffer_.Full())
        {
            buffer_.Pop();
        }

        buffer_.Push(std::format("{}  -  ", Scheduler.TotalCycles()) + message + "\n");
    }
}

std::string ConditionMnemonic(uint8_t condition)
{
    switch (condition)
    {
        case 0:
            return "EQ";
        case 1:
            return "NE";
        case 2:
            return "CS";
        case 3:
            return "CC";
        case 4:
            return "MI";
        case 5:
            return "PL";
        case 6:
            return "VS";
        case 7:
            return "VC";
        case 8:
            return "HI";
        case 9:
            return "LS";
        case 10:
            return "GE";
        case 11:
            return "LT";
        case 12:
            return "GT";
        case 13:
            return "LE";
        case 14:
            return "";
        default:
            return "";
    }
}

LogManager LogMgr;
}
