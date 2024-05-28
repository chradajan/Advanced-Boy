project(GbaLib)

target_sources(${PROJECT_NAME} PRIVATE
    EventScheduler.cpp
    GameBoyAdvance.cpp
    SystemControl.cpp
)
