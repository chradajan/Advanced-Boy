cmake_minimum_required(VERSION 3.19)

project (AdvancedBoy
    LANGUAGES CXX
    VERSION 1.0
    DESCRIPTION "Game Boy Advance Emulator"
)

set(SOURCES
    src/ARM7TDMI/ARM7TDMI.cpp
    src/ARM7TDMI/ArmInstructions.cpp
    src/ARM7TDMI/Registers.cpp
    src/ARM7TDMI/ThumbInstructions.cpp
    src/main.cpp
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

add_executable(AdvancedBoy ${SOURCES})

target_include_directories(${PROJECT_NAME}
    PRIVATE ${PROJECT_SOURCE_DIR}/include
)
