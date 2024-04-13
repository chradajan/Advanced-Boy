#pragma once

#include <filesystem>

#ifdef _LOG_PATH
    #define LOG_PATH _LOG_PATH
#else
    #define LOG_PATH ""
#endif

namespace fs = std::filesystem;

namespace Config
{
inline bool LOGGING_ENABLED;
constexpr size_t LOG_BUFFER_SIZE = 20000;
constexpr bool ASSERTS_ENABLED = true;
}
