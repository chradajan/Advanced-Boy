project(GbaLib)

target_sources(${PROJECT_NAME} PRIVATE
    EEPROM.cpp
    Flash.cpp
    GamePak.cpp
    SRAM.cpp
)
