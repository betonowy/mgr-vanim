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

file(GLOB_RECURSE DVDB_SRC src/dvdb/*.cpp)

vanim_add_benchmark(
    NAME dvdb_benchmarks
    SOURCES test/dvdb_benchmark_cases.cpp ${DVDB_SRC}
    INCLUDES src/dvdb
    LIBS glm::glm
)

vanim_add_test(
    NAME dvdb_coord_transform
    SOURCES ${DVDB_SRC}
    INCLUDES src/dvdb
)

vanim_add_test(
    NAME dvdb_dct_transform_tables
    SOURCES ${DVDB_SRC}
    INCLUDES src/dvdb src/utils
    LIBS mio::mio
)
