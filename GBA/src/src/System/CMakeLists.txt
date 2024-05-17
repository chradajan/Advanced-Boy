project(GbaLib)

target_sources(${PROJECT_NAME} PRIVATE
    DmaChannel.cpp
    GameBoyAdvance.cpp
    InterruptManager.cpp
    Scheduler.cpp
)
