#pragma once

#include <cstdint>
#include <exception>
#include <filesystem>
#include <string>
#include <vector>
#include <Config.hpp>
#include <System/InterruptManager.hpp>
#include <Utilities/CircularBuffer.hpp>

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

    /// @brief Log when an IRQ occurs.
    void LogIRQ();

    /// @brief Log when the CPU gets halted.
    /// @param ie Which interrupts are enabled at the time the halt occurs.
    void LogHalt(uint16_t IE);

    /// @brief Log when the CPU is unhalted.
    /// @param IF Which interrupts are currently requested.
    /// @param IE Which interrupts are currently enabled.
    void LogUnhalt(uint16_t IF, uint16_t IE);

    /// @brief Log when an interrupt is requested.
    /// @param interrupt Which interrupt was requested.
    void LogInterruptRequest(InterruptType interrupt);

    /// @brief Log an exception and dump logs.
    /// @param error Exception to log.
    void LogException(std::exception const& error);

    /// @brief Dump logged instructions to file.
    void DumpLogs();

private:
    /// @brief Log a string and the current cycle count.
    /// @param message Message to log.
    void LogMessage(std::string message);

    CircularBuffer<std::string, Config::LOG_BUFFER_SIZE> buffer_;

    fs::path logPath_;
    bool loggingInitialized_;
};

extern LogManager LogMgr;
}
