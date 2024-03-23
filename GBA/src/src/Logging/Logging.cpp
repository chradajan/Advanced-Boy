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
        logFile << std::format("{:08X}  {:<20}  {}\n", pc, mnemonic, registers);
    }
}
}
