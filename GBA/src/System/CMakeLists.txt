project(GbaLib)

target_sources(${PROJECT_NAME} PRIVATE
    DmaChannel.cpp
    EventScheduler.cpp
    GameBoyAdvance.cpp
    InterruptManager.cpp
)
