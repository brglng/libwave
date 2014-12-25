cmake_minimum_required(VERSION 2.6)

project(diff-wave)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
)

add_executable(${PROJECT_NAME}
    src/diff-wave.c
)

link_directories(
    ${CMAKE_BINARY_DIR}
)

target_link_libraries(${PROJECT_NAME}
    wave_static
)
