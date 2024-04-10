#pragma once

#include <cstdint>
#include <filesystem>

namespace fs = std::filesystem;

/// @brief Initialize the GBA before use.
/// @param[in] biosPath Path to GBA BIOS file.
void Initialize(fs::path biosPath);

/// @brief Load a GBA ROM.
/// @param[in] romPath GBA ROM file to be loaded.
/// @pre Initialize must have been previously called.
bool InsertCartridge(fs::path romPath);

/// @brief Run the GBA indefinitely.
/// @pre Initialize must have been previously called and a GamePak must have been successfully loaded.
void PowerOn();

/// @brief Access the raw frame buffer data.
/// @return Raw pointer to frame buffer.
/// @pre Initialize must have been previously called.
uint8_t* GetRawFrameBuffer();
