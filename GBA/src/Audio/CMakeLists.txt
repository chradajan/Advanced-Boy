project(GbaLib)

target_sources(${PROJECT_NAME} PRIVATE
    APU.cpp
    Channel1.cpp
    Channel2.cpp
    Channel4.cpp
    DmaAudio.cpp
)
