cmake_minimum_required(VERSION 3.19)

project (AdvancedBoy
    LANGUAGES CXX
    VERSION 1.0
    DESCRIPTION "Game Boy Advance Emulator"
)

set(ARM7TDMI_SOURCES
    src/ARM7TDMI/ARM7TDMI.cpp
    src/ARM7TDMI/ArmInstructions.cpp
    src/ARM7TDMI/Registers.cpp
    src/ARM7TDMI/ThumbInstructions.cpp
)

set(MAIN_SOURCES
    src/GameBoyAdvance.cpp
    src/main.cpp
)

set(MEMORY_SOURCES
    src/Memory/GamePak.cpp
    src/Memory/MemoryManager.cpp
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

add_library(${PROJECT_NAME} SHARED ${ARM7TDMI_SOURCES} ${MAIN_SOURCES} ${MEMORY_SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    PUBLIC_HEADER ${PROJECT_SOURCE_DIR}/AdvancedBoy.hpp
    RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib
    PREFIX ""
)

target_include_directories(${PROJECT_NAME}
    PRIVATE ${PROJECT_SOURCE_DIR}/include
    PUBLIC ${PROJECT_SOURCE_DIR}
)
