# Advanced Boy

This is a WIP Game Boy Advanced emulator written in C++.

## Requirements

- [CMake 3.19+](https://cmake.org/)
- [Qt6](https://www.qt.io/download-qt-installer-oss)
- C++ compiler with C++20 support

## Build Steps

Linux/Mac
```
git checkout https://github.com/chradajan/Advanced-Boy.git
cd Advanced-Boy/GBA
mkdir build
cd build
cmake ..
make
```

MinGW

```
git checkout https://github.com/chradajan/Advanced-Boy.git
cd Advanced-Boy/GBA
mkdir build
cd build
cmake -G "MinGW Makefiles" -D CMAKE_PREFIX_PATH=C:/Qt/6.6.0/mingw_64 ..
mingw32-make
```
