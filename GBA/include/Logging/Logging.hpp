#pragma once

#include <cstdint>
#include <exception>
#include <filesystem>
#include <string>
#include <vector>
#include <DMA/DmaChannel.hpp>
#include <System/SystemControl.hpp>
#include <Utilities/CircularBuffer.hpp>

namespace fs = std::filesystem;

namespace Logging
{
constexpr size_t LOG_BUFFER_SIZE = 100'000;

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

    /// @brief Toggle system event logging on/off.
    void ToggleSystemLogging() { systemLoggingEnabled_ = !systemLoggingEnabled_; }

    /// @brief Toggle CPU instruction logging on/off.
    void ToggleCpuLogging() { cpuLoggingEnabled_ = !cpuLoggingEnabled_; }

    /// @brief Check if system logging is currently enabled.
    /// @return True if system events should be logged.
    bool SystemLoggingEnabled() const { return systemLoggingEnabled_; }

    /// @brief Check if CPU logging is currently enabled.
    /// @return True if CPU instructions should be logged.
    bool CpuLoggingEnabled() const { return cpuLoggingEnabled_; }

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
    /// @param IE Which interrupts are currently enabled.
    /// @param IME Interrupt master enable status.
    void LogInterruptRequest(InterruptType interrupt, uint16_t IE, uint16_t IME);

    /// @brief Log an exception and dump logs.
    /// @param error Exception to log.
    void LogException(std::exception const& error);

    /// @brief Log when a DMA transfer occurs.
    /// @param index Index of DMA channel that's about to run.
    /// @param xferType What triggered this DMA transfer.
    /// @param src Source address.
    /// @param dest Destination address.
    /// @param cnt Number of words to transfer.
    void LogDmaTransfer(int index, DmaXfer xferType, uint32_t src, uint32_t dest, uint32_t cnt);

    /// @brief Log when one of the timer overflows.
    /// @param index Index of timer that overflowed.
    void LogTimerOverflow(int index);

    /// @brief Dump logged instructions to file.
    void DumpLogs();

private:
    /// @brief Log a string and the current cycle count.
    /// @param message Message to log.
    void LogMessage(std::string message);

    CircularBuffer<std::string, LOG_BUFFER_SIZE> buffer_;

    fs::path logPath_;
    bool loggingInitialized_;
    bool systemLoggingEnabled_;
    bool cpuLoggingEnabled_;
};

extern LogManager LogMgr;
}
