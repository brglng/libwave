# libwav

libwav is a simple and tiny C library for reading or writing PCM wave (.wav)
files.

## Build

Linux/Cygwin/OS X using makefiles:

```sh
mkdir build
cd build
cmake [-DCMAKE_BUILD_TYPE=<Debug|Release|RelWithDebInfo|MinSizeRel>] -G "Unix Makefiles" ..
make
```

Windows using Visual Studio 2013:

```sh
mkdir build
cd build
cmake [-DCMAKE_BUILD_TYPE=<Debug|Release|RelWithDebInfo|MinSizeRel>] -G "Visual Studio 12" ..
```

OS X using Xcode:

```sh
mkdir build
cd build
cmake [-DCMAKE_BUILD_TYPE=<Debug|Release|RelWithDebInfo|MinSizeRel>] -G Xcode ..
```
