project(GbaLib)

target_sources(${PROJECT_NAME} PRIVATE
    ArmInstructionLogging.cpp
    Logging.cpp
    ThumbInstructionLogging.cpp
)
