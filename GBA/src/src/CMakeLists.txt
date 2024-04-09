project(GbaLib)

target_sources(${PROJECT_NAME} PRIVATE AdvancedBoy.cpp)

add_subdirectory(ARM7TDMI)
add_subdirectory(Logging)
add_subdirectory(Memory)
add_subdirectory(System)
