# Vanim

NanoVDB animation converter/renderer and custom NanoVDB compatible format with lossy compression.
Written for as part of master thesis at TUL.

# How to build and run

Requirements:
- LLVM/Clang (May work with MinGW/GNU as well)
- CMake

Then this should work:

```
cmake -S . -B build
cmake --build build -j --target vanim-app
```

If low on memory, omit `-j` option.

By default it should work with LLVM/clang compiler on windows.

App output directory depends on chosen generator, but usually can be found as `build/app/vanim-app` or `build/app/<BUILD_TYPE>/vanim-app`. `res` directory must be inside, or a parent directory of a directory where executable is run from (if you stay within the repo this is always true). Shaders will not be found otherwise.

Program can convert directories containing enumerated `.vdb` files to convert them to `.nvdb` files. Then they can be converted to `.dvdb` files.

Troubleshooting and other info:

- Initialize and update all git submodules?
- Dependencies are acquired through vcpkg or via git submodules in thirdparty directory.
- GNU GCC/LLVM Clang on Fedora linux works. Clang/LLVM + CMake w/ Ninja generator works on Windows. MSVC is not recommended. MinGW/MSYS2 untested, but should work as well.
- Vanim application should find `res` folder itself as long as its in the same or any of the parent directories.
- Even "easier" way to build is to use VSCode with popular CMake integration extensions which detect available toolchains and configurations automatically. This repo copy should work with such setup if all required software is installed.
