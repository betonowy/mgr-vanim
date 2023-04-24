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
