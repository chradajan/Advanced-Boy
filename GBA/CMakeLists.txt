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

set(GUI_SOURCES
    include/GUI/DebugWindow.hpp
    src/GUI/DebugWindow.cpp
    include/GUI/MemoryViewerWidget.hpp
    src/GUI/MemoryViewerWidget.cpp
    include/GUI/MainWindow.hpp
    src/GUI/MainWindow.cpp
    include/GUI/RegisterViewerWidget.hpp
    src/GUI/RegisterViewerWidget.cpp
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

include(FetchContent)
Set(FETCHCONTENT_QUET FALSE)

FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL
        GIT_TAG release-2.28.4
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(SDL3)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

qt_add_executable(${PROJECT_NAME}
    ${ARM7TDMI_SOURCES}
    ${GUI_SOURCES}
    ${MAIN_SOURCES}
    ${MEMORY_SOURCES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets)

target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2main SDL2::SDL2-static)

target_include_directories(${PROJECT_NAME}
    PRIVATE ${PROJECT_SOURCE_DIR}/include
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)
