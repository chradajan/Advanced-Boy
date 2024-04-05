#pragma once

#include <cstdint>
#include <string>

namespace Logging
{
void InitializeLogging();

void LogInstruction(uint32_t pc, std::string mnemonic, std::string registers);

/// @brief Convert an ARM condition into its mnemonic.
/// @param condition ARM condition code.
/// @return ARM condition mnemonic.
std::string ConditionMnemonic(uint8_t condition);
}
