cmake_minimum_required(VERSION 3.19)

project (AdvancedBoy
    LANGUAGES CXX
    VERSION 1.0
    DESCRIPTION "Game Boy Advance Emulator"
)

set(GBA_LIBRARY GbaLib)
set(GBA_PROGRAM ${PROJECT_NAME})

set(ARM7TDMI_SOURCES
    src/ARM7TDMI/ARM7TDMI.cpp
    src/ARM7TDMI/ArmInstructions.cpp
    src/ARM7TDMI/Registers.cpp
    src/ARM7TDMI/ThumbInstructions.cpp
)

set(MAIN_SOURCES
    src/AdvancedBoy.cpp
    src/GameBoyAdvance.cpp
)

set(MEMORY_SOURCES
    src/Memory/GamePak.cpp
    src/Memory/MemoryManager.cpp
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

add_library(${GBA_LIBRARY} SHARED ${ARM7TDMI_SOURCES} ${MAIN_SOURCES} ${MEMORY_SOURCES})
add_executable(${GBA_PROGRAM} src/main.cpp)

target_link_libraries(${GBA_PROGRAM} ${GBA_LIBRARY})

set_target_properties(${GBA_LIBRARY} PROPERTIES
    VERSION ${PROJECT_VERSION}
    PUBLIC_HEADER ${PROJECT_SOURCE_DIR}/AdvancedBoy.hpp
    PREFIX ""
)

target_include_directories(${GBA_LIBRARY}
    PRIVATE ${PROJECT_SOURCE_DIR}/include
    PUBLIC ${PROJECT_SOURCE_DIR}
)
