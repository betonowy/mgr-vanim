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
    SOURCES 
        test/dvdb_benchmark_cases.cpp
        src/utils/resource_path.cpp
        src/utils/nvdb_mmap.cpp
        src/dvdb/dct.cpp
        src/dvdb/statistics.cpp
        src/dvdb/transform.cpp
        src/dvdb/derivative.cpp
        src/dvdb/rotate.cpp
        src/dvdb/compression.cpp
        src/dvdb/common.cpp
    INCLUDES src/dvdb src/utils
    LIBS glm::glm nanovdb lz4::lz4
)

vanim_add_test(
    NAME dvdb_transform
    SOURCES src/dvdb/transform.cpp
    INCLUDES src/dvdb
)

vanim_add_test(
    NAME dvdb_dct
    SOURCES
        src/dvdb/dct.cpp
        src/dvdb/statistics.cpp
        src/dvdb/transform.cpp
        src/dvdb/derivative.cpp
        src/dvdb/common.cpp
    INCLUDES src/dvdb src/utils
    LIBS mio::mio glm::glm
)

vanim_add_test(
    NAME dvdb_derivative
    SOURCES src/dvdb/derivative.cpp
    INCLUDES src/dvdb
)

vanim_add_test(
    NAME dvdb_compression
    SOURCES
        src/utils/resource_path.cpp
        src/utils/nvdb_mmap.cpp
        src/dvdb/compression.cpp
        src/dvdb/derivative.cpp
        src/dvdb/dct.cpp
        src/dvdb/statistics.cpp
        src/dvdb/transform.cpp
        src/dvdb/common.cpp
    INCLUDES src/dvdb src/utils
    LIBS lz4::lz4 nanovdb
)

vanim_add_test(
    NAME dvdb_rotate
    SOURCES 
        src/utils/resource_path.cpp
        src/utils/nvdb_mmap.cpp
        src/dvdb/dct.cpp
        src/dvdb/statistics.cpp
        src/dvdb/transform.cpp
        src/dvdb/derivative.cpp
        src/dvdb/rotate.cpp
        src/dvdb/common.cpp
        src/dvdb/compression.cpp
    INCLUDES src/dvdb src/utils
    LIBS mio::mio nanovdb glm::glm
)

vanim_add_test(
    NAME dvdb_statistics
    SOURCES src/dvdb/statistics.cpp src/dvdb/common.cpp
    INCLUDES src/dvdb
)
