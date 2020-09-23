# libwav

libwav is a simple and tiny C library for reading or writing PCM wave (.wav)
files.

## Build

    mkdir build
    cd build
    cmake [-DCMAKE_BUILD_TYPE=<Debug|Release|RelWithDebInfo>] ..
    make

## CMake Support

Use `FetchContent`:

    include(FetchContent)
    FetchContent_Declare(naivedsp
        GIT_REPOSITORY    "https://github.com/brglng/libwav.git" 
        GIT_SHALLOW       ON
        )
    FetchContent_MakeAvailable(naivedsp)
    add_executable(yourprogram yourprogram.c)
    target_link_libraries(yourprogram wav::wav)

Use `add_subdirectory`:

    add_subdirectory(naivedsp)
    add_executable(yourprogram yourprogram.c)
    target_link_libraries(yourprogram wav::wav)

Use `find_package`:

    find_package(naivedsp)
    add_executable(yourprogram yourprogram.c)
    target_link_libraries(yourprogram wav::wav)
