libwave
=======

libwave is a simple and tiny C library for reading or writing PCM wave (.wav)
files.

Build
-----

Linux/Cygwin/OS X using makefiles:

```sh
mkdir build
cd build
cmake [-DCMAKE_BUILD_TYPE=<Debug|Release>] -G "Unix Makefiles" ..
make
```

Windows using Visual Studio 2013:

```sh
mkdir build
cd build
cmake [-DCMAKE_BUILD_TYPE=<Debug|Release>] -G "Visual Studio 12" ..
```

OS X using Xcode:

```sh
mkdir build
cd build
cmake [-DCMAKE_BUILD_TYPE=<Debug|Release>] -G Xcode ..
```

License
-------
This library is released under
[LGPL V3](https://www.gnu.org/licenses/lgpl.html).
