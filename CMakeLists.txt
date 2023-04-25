cmake_minimum_required(VERSION 3.24)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(WIN32)
    set(VCPKG_DEFAULT_TRIPLET x64-windows-static)
    set(VCPKG_TARGET_TRIPLET x64-windows-static)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_compile_definitions(_ITERATOR_DEBUG_LEVEL=0)
    
    add_compile_options("$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")
    add_compile_options("$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/Zc:throwingNew>")
    add_compile_options("$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/Zc:inline>")
endif()

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

project(Vanim)

include(cmake/deps.cmake)
include(cmake/tests.cmake)

add_subdirectory(thirdparty)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(app)
add_subdirectory(lib)
