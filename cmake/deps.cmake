find_package(SDL2 CONFIG REQUIRED)
find_package(gl3w CONFIG REQUIRED)
find_package(zstd CONFIG REQUIRED)
find_package(lz4 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(mio CONFIG REQUIRED)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/openvdb/nanovdb/nanovdb/PNanoVDB.h ${CMAKE_CURRENT_SOURCE_DIR}/res/glsl/pnanovdb.glsl COPYONLY)

find_program(MOLD_EXECUTABLE mold NO_CACHE)

if(MOLD_EXECUTABLE)
    message(STATUS "Using mold linker: ${MOLD_EXECUTABLE}")
    set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> -fuse-ld=mold <FLAGS> <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
else()
    message(STATUS "Couldn't find mold linker: using defaults")
endif()
