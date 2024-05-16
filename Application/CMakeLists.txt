project(AdvancedBoy)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2)
find_package(SDL2 REQUIRED CONFIG REQUIRED COMPONENTS SDL2main)

qt_standard_project_setup()

qt_add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE
    main.cpp
)

add_subdirectory(include)
add_subdirectory(src)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    COMPILE_FLAGS "-Wall -Wextra -O2 -g"
)

target_include_directories(${PROJECT_NAME}
    PRIVATE ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    GbaLib
    SDL2::SDL2main
    SDL2::SDL2
    Qt6::Core
    Qt6::Widgets
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)
