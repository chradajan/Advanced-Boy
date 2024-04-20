#pragma once

#include <Gamepad.hpp>
#include <cstdint>
#include <filesystem>
#include <functional>

namespace fs = std::filesystem;

/// @brief Initialize the GBA before use.
/// @param[in] biosPath Path to GBA BIOS file.
/// @param[in] refreshScreenCallback Function callback to use when screen is ready to be refreshed.
void Initialize(fs::path biosPath, std::function<void(int)> refreshScreenCallback);

/// @brief Load a GBA ROM.
/// @param[in] romPath GBA ROM file to be loaded.
/// @pre Initialize must have been previously called.
bool InsertCartridge(fs::path romPath);

/// @brief Run the GBA indefinitely.
/// @pre Initialize must have been previously called and a GamePak must have been successfully loaded.
void PowerOn();

/// @brief Update the GBA gamepad status.
/// @param gamepad Current gamepad buttons being pressed.
/// @pre Initialize must have been previously called.
void UpdateGamepad(Gamepad gamepad);

/// @brief Access the raw frame buffer data.
/// @return Raw pointer to frame buffer.
/// @pre Initialize must have been previously called.
uint8_t* GetRawFrameBuffer();

/// @brief Enable/Disable logging of CPU instructions.
/// @param enable Whether to enable or disable logging.
void EnableLogging(bool enable);
