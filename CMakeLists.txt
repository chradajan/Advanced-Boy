cmake_minimum_required(VERSION 3.19)

project (AdvancedBoy
    LANGUAGES CXX
    VERSION 1.0
    DESCRIPTION "Game Boy Advance Emulator"
)

add_definitions(-D_LOG_PATH="${PROJECT_SOURCE_DIR}/logs")
add_definitions(-D_BIOS_PATH="${PROJECT_SOURCE_DIR}/bios/gba_bios.bin")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

add_subdirectory(GBA)
add_subdirectory(Application)
