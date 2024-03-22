project(GbaLib)

set(GBA_SOURCES
    src/src/AdvancedBoy.cpp
    src/src/ARM7TDMI/ARM7TDMI.cpp
    src/src/ARM7TDMI/ArmInstructions.cpp
    src/src/ARM7TDMI/Registers.cpp
    src/src/ARM7TDMI/ThumbInstructions.cpp
    src/src/Memory/GamePak.cpp
    src/src/Memory/MemoryManager.cpp
    src/src/System/GameBoyAdvance.cpp
)

add_library(${PROJECT_NAME} STATIC ${GBA_SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    COMPILE_FLAGS "-Wall -Wextra"
    PUBLIC_HEADER ${PROJECT_SOURCE_DIR}/AdvancedBoy.hpp
    PREFIX ""
)

target_include_directories(${PROJECT_NAME}
    PRIVATE ${PROJECT_SOURCE_DIR}/src/include
    PUBLIC ${PROJECT_SOURCE_DIR}/include
)
