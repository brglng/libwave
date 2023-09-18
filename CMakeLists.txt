cmake_minimum_required(VERSION 3.14)
list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")

# Project Information
project(wave VERSION "0.1.0")

# CMake variables that affects building
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
    if(CMAKE_BUILD_TYPE STREQUAL "")
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
include(waveTargetProperties)

add_library(${PROJECT_NAME} src/wave.c)
add_library(wave::wave ALIAS wave)
target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>
    )
target_compile_features(${PROJECT_NAME} PRIVATE ${wave_compile_features})
target_compile_definitions(${PROJECT_NAME} PRIVATE ${wave_compile_definitions})
target_compile_options(${PROJECT_NAME} PRIVATE
    ${wave_c_flags}
    $<$<CONFIG:RELEASE>:${wave_compile_options_release}>
    $<$<CONFIG:RELWITHDEBINFO>:${wave_compile_options_release}>
    )

if(BUILD_TESTING AND "${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
    add_subdirectory(tests/write_f32)
endif()

export(TARGETS wave NAMESPACE wave FILE waveTargets.cmake)

install(
    TARGETS ${PROJECT_NAME}
    EXPORT waveTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

export(PACKAGE wave)

install(DIRECTORY include DESTINATION .)

install(
    FILES README.md CHANGELOG.md LICENSE
    DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/${PROJECT_NAME}
    )

install(EXPORT waveTargets
    FILE waveTargets.cmake
    NAMESPACE wave::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
    )

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    waveConfigVersion.cmake
    VERSION ${PACKAGE_VERSION}
    COMPATIBILITY SameMajorVersion
    )

configure_file(cmake/waveConfig.cmake.in waveConfig.cmake @ONLY)
install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/waveConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/waveConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
    )
  
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
    set(CPACK_PACKAGE_NAME "wave")
    set(CPACK_GENERATOR "TXZ")
    set(CPACK_SOURCE_IGNORE_FILES
        "/\\\\.git/"
        "\\\\.git.*"
        "/build/"
        "/backup/"
        "/cmake-build-.*/"
        "/\\\\.idea/"
        "/\\\\.ycm_extra_conf\\\\..*"
        "/GPATH$"
        "/GRTAGS$"
        "/GSYMS$"
        "/GTAGS$"
        "\\\\.swp$"
        "\\\\.swo$"
        ".DS_Store"
        ".ccls"
        ".ccls-cache"
        "compile_commands.json"
        )
    set(CPACK_SOURCE_GENERATOR "TXZ")
    include(CPack)
endif()
