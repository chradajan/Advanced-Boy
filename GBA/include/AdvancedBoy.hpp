#pragma once
#include <filesystem>

namespace fs = std::filesystem;

/// @brief Initialize the GBA before use.
/// @param[in] biosPath Path to GBA BIOS file.
/// @param[in] romPath Optionally provide a path to a GBA ROM to be loaded.
void Initialize(fs::path biosPath, fs::path romPath = "");

/// @brief Load a GBA ROM.
/// @param[in] romPath GBA ROM file to be loaded.
void InsertCartridge(fs::path romPath);

/// @brief Power on GBA. Initialize must have been called before powering on.
void PowerOn();
