cmake_minimum_required(VERSION 2.8)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
)

add_executable(diff-wav
    src/diff-wav.c
)

link_directories(
    ${CMAKE_BINARY_DIR}
)

target_link_libraries(diff-wav
    wav
)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    target_link_libraries(diff-wav m)
endif()
