#pragma once

#include <Gamepad.hpp>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>

namespace fs = std::filesystem;

/// @brief Initialize the GBA before use.
/// @param[in] biosPath Path to GBA BIOS file.
/// @param[in] refreshScreenCallback Function callback to use when screen is ready to be refreshed.
void Initialize(fs::path biosPath);

/// @brief Load a GBA ROM.
/// @param[in] romPath GBA ROM file to be loaded.
/// @pre Initialize must have been previously called.
bool InsertCartridge(fs::path romPath);

/// @brief Run the emulator until the specified number of audio samples have been produced.
/// @param samples Number of audio samples to produce.
void FillAudioBuffer(int samples);

/// @brief Drain the emulator's internal audio buffer into the specified buffer.
/// @param buffer Buffer to load internal audio buffer's samples into.
void DrainAudioBuffer(float* buffer);

/// @brief Update the GBA gamepad status.
/// @param gamepad Current gamepad buttons being pressed.
/// @pre Initialize must have been previously called.
void UpdateGamepad(Gamepad gamepad);

/// @brief Access the raw frame buffer data.
/// @return Raw pointer to frame buffer.
/// @pre Initialize must have been previously called.
uint8_t* GetRawFrameBuffer();

/// @brief Get the number of times the PPU has hit VBlank since the last check.
/// @return Number of times PPU has entered VBlank.
int GetAndResetFrameCounter();

/// @brief Enable/Disable logging of CPU instructions.
/// @param enable Whether to enable or disable logging.
void EnableLogging(bool enable);

/// @brief If logging was enabled, dump the log buffer to a file.
void DumpLogs();

/// @brief Get the title of the currently loaded ROM.
/// @return Title of ROM.
/// @pre Initialize must have been previously called.
std::string RomTitle();

/// @brief Power down the GBA, creating a save file if a cartridge was loaded.
void PowerOff();
