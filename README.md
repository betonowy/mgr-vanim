# Vanim

NanoVDB animation converter/renderer and custom NanoVDB compatible format with lossy compression.

# How to build and run

- Initialize and update all git submodules.
- Dependencies are picked through Vcpkg and it's toolchain should setup itself.
- Configure cmake project and build it with your compiler and build system of choice. Clang/LLVM + CMake w/ Ninja generator works on Windows.
- Vanim application should find `res` folder itself as long as its in the same or any of the parent directories.
- Dependencies are acquired through vcpkg or via git submodules in thirdparty directory.
- Easier way to build is to use VSCode with popular CMake integration extensions which detect available toolchains and configuretions automatically. This repo copy should work with such setup if all required software is installed.
