project(GbaLib)

target_sources(${PROJECT_NAME} PRIVATE AdvancedBoy.cpp)

add_subdirectory(Audio)
add_subdirectory(Cartridge)
add_subdirectory(CPU)
add_subdirectory(DMA)
add_subdirectory(Gamepad)
add_subdirectory(Graphics)
add_subdirectory(Logging)
add_subdirectory(System)
add_subdirectory(Timers)
add_subdirectory(Utilities)
