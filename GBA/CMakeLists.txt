project(GbaLib)

add_library(${PROJECT_NAME} STATIC)

add_subdirectory(src/src)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    COMPILE_FLAGS "-Wall -Wextra -g"
    PUBLIC_HEADER ${PROJECT_SOURCE_DIR}/AdvancedBoy.hpp
    PREFIX ""
)

target_include_directories(${PROJECT_NAME}
    PRIVATE ${PROJECT_SOURCE_DIR}/src/include
    PUBLIC ${PROJECT_SOURCE_DIR}/include
)
