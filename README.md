libwave
=======

libwave is a simple and tiny C library for reading or writing PCM wave (.wav)
files.

Build
-----

Linux/Cygwin/OS X using makefiles:
    ```shell
    mkdir build
    cd build
    cmake [-DDEBUG=1] -G "Unix Makefiles" ..
    make
    ```

Windows using Visual Studio 2013
    ```shell
    mkdir build
    cd build
    cmake [-DDEBUG=1] -G "Visual Studio 12" ..
    ```

OS X using Xcode
    ```shell
    mkdir build
    cd build
    cmake [-DDEBUG=1] -G Xcode ..
    ```

License
-------
This library is released under
[LGPL V3](https://www.gnu.org/licenses/lgpl.html).
