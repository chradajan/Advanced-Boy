#pragma once

#include <array>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

namespace Logging
{
/// @brief Convert an ARM condition into its mnemonic.
/// @param condition ARM condition code.
/// @return ARM condition mnemonic.
std::string ConditionMnemonic(uint8_t condition);

class LogManager
{
public:
    /// @brief Initialize with logging disabled.
    LogManager();

    /// @brief Initialize LogManager and prepare to log.
    void Initialize();

    /// @brief Save a logged instruction into buffer.
    /// @param pc PC value of logged instruction.
    /// @param mnemonic Human readable mnemonic.
    /// @param registers 
    void LogInstruction(uint32_t pc, std::string mnemonic, std::string registers);

    /// @brief Dump logged instructions to file.
    void DumpLogs();

    /// @brief Log an exception and dump logs.
    /// @param error Exception to log.
    void LogException(std::exception const& error);

private:
    void DumpBufferToFile(size_t bufferToDump);

    std::array<std::vector<std::string>, 2> buffers_;
    size_t bufferIndex_;

    fs::path logPath_;
    bool loggingInitialized_;
};

extern LogManager LogMgr;
}
