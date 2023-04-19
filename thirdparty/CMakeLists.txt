set(OPENVDB_BUILD_NANOVDB ON)
set(NANOVDB_USE_OPENVDB ON)
set(NANOVDB_USE_TBB ON)
set(OPENVDB_CORE_SHARED OFF)
set(OPENVDB_CORE_STATIC ON)
set(USE_EXPLICIT_INSTANTIATION OFF)

find_package(blosc CONFIG REQUIRED)
add_library(Blosc::blosc ALIAS blosc_static)

add_subdirectory(openvdb SYSTEM)

target_link_libraries(openvdb_static INTERFACE zstd::libzstd_static lz4::lz4)

add_library(imgui
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/backends/imgui_impl_sdl2.cpp
    imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC imgui/)

target_link_libraries(imgui PRIVATE SDL2::SDL2-static)
