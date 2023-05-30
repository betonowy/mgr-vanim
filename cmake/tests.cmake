enable_testing()

function(vanim_add_test)
    set(singleValueArgs "NAME")
    set(multiValueArgs "SOURCES;LIBS;INCLUDES")

    cmake_parse_arguments("" "" "${singleValueArgs}" "${multiValueArgs}" ${ARGN})
    set(_TEST_NAME "${_NAME}_test")

    add_executable("${_TEST_NAME}" "test/${_NAME}.cpp" ${_SOURCES})
    target_include_directories("${_TEST_NAME}" PRIVATE ${_INCLUDES})
    target_link_libraries("${_TEST_NAME}" PRIVATE Catch2::Catch2WithMain ${_LIBS})
    add_test(NAME "${_TEST_NAME}" COMMAND "${_TEST_NAME}")
endfunction()

function(vanim_add_benchmark)
    set(singleValueArgs "NAME")
    set(multiValueArgs "SOURCES;LIBS;INCLUDES")

    cmake_parse_arguments("" "" "${singleValueArgs}" "${multiValueArgs}" ${ARGN})
    set(_TEST_NAME "${_NAME}_test")

    add_executable("${_TEST_NAME}" "test/${_NAME}.cpp" ${_SOURCES})
    target_include_directories("${_TEST_NAME}" PRIVATE ${_INCLUDES})
    target_link_libraries("${_TEST_NAME}" PRIVATE Catch2::Catch2WithMain ${_LIBS})
endfunction()
