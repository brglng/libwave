cmake_minimum_required(VERSION 2.6)

project(wave)

set(WAVE_INCLUDES
    include
)

set(WAVE_INCLUDE_FILES
    include/wave.h
    include/wave_defs.h
)

set(WAVE_SOURCES
    ${WAVE_INCLUDES_FILES}
    src/wave.c
    src/wave_priv.h
)

set(WAVE_SOVERSION  0)
set(WAVE_VERSION    0.1.0)

add_library(${PROJECT_NAME} SHARED ${WAVE_SOURCES})
set_target_properties(${PROJECT_NAME}
    PROPERTIES
    VERSION                 ${WAVE_VERSION}
    SOVERSION               ${WAVE_SOVERSION}
)
target_include_directories(
    ${PROJECT_NAME}
    PUBLIC ${WAVE_INCLUDES}
)

add_library(${PROJECT_NAME}_static STATIC ${WAVE_SOURCES})
set_target_properties(${PROJECT_NAME}_static
    PROPERTIES
    VERSION                 ${WAVE_VERSION}
    SOVERSION               ${WAVE_SOVERSION}
)
target_include_directories(
    ${PROJECT_NAME}_static
    PUBLIC ${WAVE_INCLUDES}
)

install(
    TARGETS ${PROJECT_NAME} ${PROJECT_NAME}_static
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(
    FILES       ${WAVE_INCLUDE_FILES}
    DESTINATION include/${PROJECT_NAME}
)

if("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Wall")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} /Zi")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} /O2")
elseif(("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU") OR ("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang"))
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -W -Wall -Wno-multichar -std=c11")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -g -O0")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -O2")
endif()

if (WIN32)
    add_subdirectory(
        app/diff-wave
    )
endif(WIN32)
