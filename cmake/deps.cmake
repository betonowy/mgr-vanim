find_package(SDL2 CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)
find_package(gl3w CONFIG REQUIRED)
find_package(zstd CONFIG REQUIRED)
find_package(lz4 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/openvdb/nanovdb/nanovdb/PNanoVDB.h ${CMAKE_CURRENT_SOURCE_DIR}/res/glsl/pnanovdb.glsl COPYONLY)
