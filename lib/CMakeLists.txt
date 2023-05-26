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

add_executable(dvdb_benchmarks test/dvdb_benchmarks.cpp test/dvdb_benchmark_cases.cpp ${DVDB_SRC})
target_include_directories(dvdb_benchmarks PRIVATE src/dvdb)
target_link_libraries(dvdb_benchmarks PRIVATE Catch2::Catch2WithMain glm::glm)

add_test(NAME dvdb_benchmarks COMMAND dvdb_benchmarks)
