add_executable(csv_to_graphs csv_to_graphs.cpp)
target_link_libraries(csv_to_graphs PRIVATE sciplot::sciplot)

add_executable(compare_trees compare_trees.cpp)
target_link_libraries(compare_trees PRIVATE nanovdb vanim)
target_include_directories(compare_trees PRIVATE ../lib/src)
