project(GbaLib)

target_sources(${PROJECT_NAME} PRIVATE
    ARM7TDMI.cpp
    ArmInstructions.cpp
    Registers.cpp
    ThumbInstructions.cpp
)
