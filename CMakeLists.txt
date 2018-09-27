cmake_minimum_required(VERSION 2.8)

project(wav)

set(WAV_VERSION_MAJOR 0)
set(WAV_VERSION_MINOR 1)
set(WAV_VERSION_PATCH 0)
set(WAV_VERSION 0.1.0)
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 1)
set(PROJECT_VERSION_PATCH 0)
set(PROJECT_VERSION 0.1.0)

set(includes
  include
)

set(public_headers
  include/wav.h
  include/wav_defs.h
)

set(sources
  src/wav.c
  src/wav_priv.h
)

add_library(${PROJECT_NAME} ${sources})

target_include_directories(${PROJECT_NAME} PUBLIC ${includes})

install(
  TARGETS ${PROJECT_NAME} ${PROJECT_NAME}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

install(
  FILES       ${public_headers}
  DESTINATION include/${PROJECT_NAME}
)

if("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Wall")
elseif("${CMAKE_C_COMPILER_ID}" MATCHES "^(GNU|.*Clang)$")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -W -Wall -Wno-multichar -std=c99")
endif()

if(WIN32)
  add_subdirectory(
    diff-wav
  )
endif()
