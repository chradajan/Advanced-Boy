#include <Logging/Logging.hpp>
#include <Config.hpp>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>

namespace fs = std::filesystem;
static std::mutex mtx;

namespace Logging
{
LogManager::LogManager() :
    bufferIndex_(0),
    loggingInitialized_(false)
{
    logPath_ = LOG_PATH;

    if (!logPath_.empty() && Config::LOGGING_ENABLED)
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
    if (loggingInitialized_)
    {
        buffers_.at(bufferIndex_).push_back(std::format("{:08X}:  {:<40}  {}\n", pc, mnemonic, registers));

        if (buffers_.at(bufferIndex_).size() == Config::LOG_BUFFER_SIZE)
        {
            DumpLogs();
        }
    }
}

void LogManager::DumpLogs()
{
    if (loggingInitialized_)
    {
        size_t bufferToDump = bufferIndex_;
        bufferIndex_ = (bufferIndex_ == 1) ? 0 : 1;
        std::thread dumpThread(&LogManager::DumpBufferToFile, this, bufferToDump);
        dumpThread.detach();
    }
}

void LogManager::LogException(std::runtime_error const& error)
{
    if (loggingInitialized_)
    {
        buffers_.at(bufferIndex_).push_back(error.what());

        if (buffers_.at(bufferIndex_).size() == Config::LOG_BUFFER_SIZE)
        {
            DumpLogs();
        }
    }
}

void LogManager::DumpBufferToFile(size_t bufferToDump)
{
    if (loggingInitialized_)
    {
        std::lock_guard<std::mutex> lock(mtx);
        std::ofstream logFile;
        logFile.open(logPath_, std::ios_base::app);

        for (auto const& instruction : buffers_.at(bufferToDump))
        {
            logFile << instruction;
        }

        buffers_.at(bufferToDump).clear();
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
