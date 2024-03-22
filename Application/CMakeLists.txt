project(AdvancedBoy)

set(APPLICATION_SOURCES
    main.cpp
)

add_executable(${PROJECT_NAME} ${APPLICATION_SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    COMPILE_FLAGS "-Wall -Wextra"
)

target_link_libraries(${PROJECT_NAME}
    GbaLib
)
