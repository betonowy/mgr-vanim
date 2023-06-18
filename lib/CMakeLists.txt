file(GLOB_RECURSE VANIM_SRC src/*.cpp)

add_library(vanim ${VANIM_SRC})

target_link_libraries(vanim
    PRIVATE
        SDL2::SDL2-static
        unofficial::gl3w::gl3w
        glm::glm
        openvdb_static
        nanovdb
        imgui
        lz4::lz4
)

target_include_directories(vanim
    INTERFACE
        "include"
    PRIVATE
        "include"
        "src"
)

target_compile_definitions(vanim
    PRIVATE
        "PNANOVDB_C"
)

if(WIN32)
    target_compile_definitions(vanim
        PRIVATE
            VANIM_WINDOWS
    )
else()
    target_compile_definitions(vanim
        PRIVATE
            VANIM_UNIX
    )
endif()

vanim_add_benchmark(
    NAME dvdb_benchmarks
    INCLUDES src/dvdb src/utils src
    LIBS glm::glm nanovdb lz4::lz4 vanim
)

vanim_add_test(
    NAME dvdb_transform
    INCLUDES src/dvdb src vanim
    LIBS vanim
)

vanim_add_test(
    NAME dvdb_dct
    INCLUDES src/dvdb src/utils src
    LIBS mio::mio glm::glm vanim
)

vanim_add_test(
    NAME dvdb_derivative
    INCLUDES src/dvdb src
    LIBS vanim
)

vanim_add_test(
    NAME dvdb_compression
    INCLUDES src/dvdb src/utils src
    LIBS lz4::lz4 nanovdb vanim
)

vanim_add_test(
    NAME dvdb_rotate
    INCLUDES src/dvdb src/utils src
    LIBS mio::mio nanovdb glm::glm vanim
)

vanim_add_test(
    NAME dvdb_statistics
    INCLUDES src/dvdb src
    LIBS vanim
)
