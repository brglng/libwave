cmake_minimum_required(VERSION 3.14)
if(POLICY CMP0025)
  cmake_policy(SET CMP0025 NEW)
endif()
if(POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
endif()
if(POLICY CMP0054)
  cmake_policy(SET CMP0054 NEW)
endif()
if(POLICY CMP0077)
  cmake_policy(SET CMP0077 NEW)
endif()
list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")

# Project Information
project(wav C)
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 1)
set(PROJECT_VERSION_PATCH 0)
set(PROJECT_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
set(${PROJECT_NAME}_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(${PROJECT_NAME}_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(${PROJECT_NAME}_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(${PROJECT_NAME}_VERSION ${PROJECT_VERSION})

# CMake variables that affects building
if(${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
  if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
  endif()

  if(NOT DEFINED BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS ON CACHE BOOL "enable building of shared libraries instead of static ones" FORCE)
  endif()

  if(NOT DEFINED CMAKE_POSITION_INDEPENDENT_CODE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON CACHE BOOL "enable position independent code" FORCE)
  endif()

  include(CTest)
  message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
  message(STATUS "CMAKE_INSTALL_PREFIX: ${CMAKE_INSTALL_PREFIX}")
  message(STATUS "CMAKE_POSITION_INDEPENDENT_CODE: ${CMAKE_POSITION_INDEPENDENT_CODE}")
  message(STATUS "BUILD_SHARED_LIBS: ${BUILD_SHARED_LIBS}")
  message(STATUS "BUILD_TESTING: ${BUILD_TESTING}")
endif()

include(GNUInstallDirs)
include(wavTargetProperties)

add_library(${PROJECT_NAME} src/wav.c)
add_library(wav::wav ALIAS wav)
target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
  )
target_compile_features(${PROJECT_NAME} PRIVATE ${wav_compile_features})
target_compile_definitions(${PROJECT_NAME} PRIVATE ${wav_compile_definitions})
target_compile_options(${PROJECT_NAME} PRIVATE
  ${wav_c_flags}
  $<$<CONFIG:RELEASE>:${wav_compile_options_release}>
  $<$<CONFIG:RELWITHDEBINFO>:${wav_compile_options_release}>
  )

export(TARGETS wav NAMESPACE wav FILE wavTargets.cmake)

install(
  TARGETS ${PROJECT_NAME}
  EXPORT wavTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )

export(PACKAGE wav)

if(${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
  if(WIN32)
    add_subdirectory(diff-wav)
  endif()
endif()

install(DIRECTORY include DESTINATION .)

install(
  FILES README.md CHANGELOG.md LICENSE
  DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/${PROJECT_NAME}
  )

install(EXPORT wavTargets
  FILE wavTargets.cmake
  NAMESPACE wav::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
  )

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  wavConfigVersion.cmake
  VERSION ${PACKAGE_VERSION}
  COMPATIBILITY SameMajorVersion
  )

configure_file(cmake/wavConfig.cmake.in wavConfig.cmake @ONLY)
install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/wavConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/wavConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
  )
  
if(${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
  set(CPACK_PACKAGE_NAME "wav")
  set(CPACK_GENERATOR "TXZ")
  set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
  set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
  set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
  set(CPACK_SOURCE_IGNORE_FILES "/\\\\.git/;\\\\.git.*;/build/;/backup/;/cmake-build-.*/;/\\\\.idea/;/\\\\.ycm_extra_conf\\\\..*;/GPATH$;/GRTAGS$;/GSYMS$;/GTAGS$;\\\\.swp$;\\\\.swo$;.DS_Store;.ccls;.ccls-cache")
  set(CPACK_SOURCE_GENERATOR "TXZ")
  include(CPack)
endif()
