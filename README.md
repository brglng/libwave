---

*This fork adds build support for ARM platforms and fixes compiling issues*

---

# libwav

libwav is a simple and tiny C library for reading or writing PCM wave (.wav)
files.

## Build and Install

On Linux and macOS:

    mkdir build
    cd build
    cmake [-DCMAKE_BUILD_TYPE=<Debug|Release|RelWithDebInfo>] ..
    make
    sudo make install

On Windows:

    mkdir build
    cd build
    cmake ..
    cmake --build .

## CMake Support

Use `FetchContent`:

    include(FetchContent)
    FetchContent_Declare(libwav
        GIT_REPOSITORY    "https://github.com/brglng/libwav.git" 
        GIT_SHALLOW       ON
        )
    FetchContent_MakeAvailable(libwav)
    add_executable(yourprogram yourprogram.c)
    target_link_libraries(yourprogram wav::wav)

Use `add_subdirectory`:

    add_subdirectory(libwav)
    add_executable(yourprogram yourprogram.c)
    target_link_libraries(yourprogram wav::wav)

Use `find_package`:

    find_package(wav)
    add_executable(yourprogram yourprogram.c)
    target_link_libraries(yourprogram wav::wav)
