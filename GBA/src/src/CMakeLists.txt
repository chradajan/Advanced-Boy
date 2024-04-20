project(GbaLib)

target_sources(${PROJECT_NAME} PRIVATE AdvancedBoy.cpp)

add_subdirectory(ARM7TDMI)
add_subdirectory(Cartridge)
add_subdirectory(Gamepad)
add_subdirectory(Graphics)
add_subdirectory(Logging)
add_subdirectory(System)
