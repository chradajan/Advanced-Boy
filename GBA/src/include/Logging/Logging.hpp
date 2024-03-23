#pragma once

#include <cstdint>
#include <string>

namespace Logging
{
void InitializeLogging();

void LogInstruction(uint32_t pc, std::string mnemonic, std::string registers);
}
