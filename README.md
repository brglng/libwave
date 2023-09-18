# libwave

libwave is a simple and tiny C library for reading or writing PCM wave (.wav)
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
    FetchContent_Declare(libwave
        GIT_REPOSITORY    "https://github.com/brglng/libwave.git" 
        GIT_SHALLOW       ON
        )
    FetchContent_MakeAvailable(libwave)
    add_executable(yourprogram yourprogram.c)
    target_link_libraries(yourprogram wave::wave)

Use `add_subdirectory`:

    add_subdirectory(libwave)
    add_executable(yourprogram yourprogram.c)
    target_link_libraries(yourprogram wave::wave)

Use `find_package`:

    find_package(wave)
    add_executable(yourprogram yourprogram.c)
    target_link_libraries(yourprogram wave::wave)
