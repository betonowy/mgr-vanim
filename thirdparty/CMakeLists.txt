set(OPENVDB_BUILD_NANOVDB ON)
set(NANOVDB_USE_OPENVDB ON)
set(NANOVDB_USE_TBB ON)
set(OPENVDB_CORE_SHARED OFF)
set(OPENVDB_CORE_STATIC ON)
set(USE_EXPLICIT_INSTANTIATION OFF)

find_package(blosc CONFIG REQUIRED)
add_library(Blosc::blosc ALIAS blosc)

find_package(Snappy CONFIG REQUIRED)
get_target_property(BLOSC_LINKED_LIBRARIES blosc INTERFACE_LINK_LIBRARIES)
list(TRANSFORM BLOSC_LINKED_LIBRARIES REPLACE .+ $<TARGET_FILE:Snappy::snappy> REGEX /lib/snappy.)
set_property(TARGET blosc PROPERTY INTERFACE_LINK_LIBRARIES "${BLOSC_LINKED_LIBRARIES}")

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

add_subdirectory(catch2)

get_target_property(TARGET_TYPE lz4::lz4 TYPE)

if (TARGET_TYPE STREQUAL STATIC_LIBRARY)
    target_compile_definitions(lz4::lz4 INTERFACE LZ4_HC_STATIC_LINKING_ONLY)
endif()

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(openvdb EXCLUDE_FROM_ALL SYSTEM)
target_link_libraries(openvdb_static INTERFACE zstd::libzstd_static lz4::lz4)
target_compile_definitions(nanovdb INTERFACE PNANOVDB_C)
