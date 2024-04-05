#include <Logging/Logging.hpp>
#include <Config.hpp>
#include <cstdint>
#include <format>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

fs::path logPath;
bool loggingInitialized = false;

namespace Logging
{
void InitializeLogging()
{
    logPath = LOG_PATH;

    if (!logPath.empty() && Config::LOGGING_ENABLED)
    {
        if (!fs::exists(logPath))
        {
            fs::create_directory(logPath);
        }

        logPath /= "log.log";

        if (fs::exists(logPath))
        {
            fs::remove(logPath);
        }

        loggingInitialized = true;
    }
}

void LogInstruction(uint32_t const pc, std::string const mnemonic, std::string const registers)
{
    if (loggingInitialized)
    {
        std::ofstream logFile;
        logFile.open(logPath, std::ios_base::app);
        logFile << std::format("{:08X}:  {:<40}  {}\n", pc, mnemonic, registers);
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
}
