#pragma once

#include <filesystem>

#ifdef _LOG_PATH
    #define LOG_PATH _LOG_PATH
#else
    #define LOG_PATH ""
#endif

#ifdef _BIOS_PATH
    #define BIOS_PATH _BIOS_PATH
#else
    #define BIOS_PATH ""
#endif

namespace fs = std::filesystem;
