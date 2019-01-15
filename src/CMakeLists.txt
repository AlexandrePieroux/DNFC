# Define library
project(dnfclib)

# Recursively get all source files
file(GLOB_RECURSE SOURCES 
    "*.cpp"
)

# Get header files
file(GLOB_RECURSE HEADERS 
    "*.hpp"
)

# Recursively get all cpp files of the test
file(GLOB_RECURSE TEST_SOURCES 
    "*_test.cpp"
)

# Remove tests from source files
foreach(test ${TEST_SOURCES})
    list(REMOVE_ITEM SOURCES ${test}) 
endforeach(test)

# Get include directories
foreach(header_file ${HEADERS})
    get_filename_component(_dir ${header_file} PATH)
    list (APPEND INCLUDE_DIR ${_dir})
endforeach(header_file)
list(REMOVE_DUPLICATES INCLUDE_DIR)

# Scan for tests
foreach(test ${TEST_SOURCES})

    # Get test name
    get_filename_component(TESTNAME ${test} NAME)
    string(REGEX MATCH "^(.*)\\.[^.]*$" dummy ${TESTNAME})
    set(TESTNAME ${CMAKE_MATCH_1})
    message(STATUS "Test found: " ${TESTNAME})

    # Create test executable
    add_executable(${TESTNAME} ${test})
    target_include_directories(${TESTNAME} PRIVATE ${INCLUDE_DIR})
    target_link_libraries(${TESTNAME} 
                          gtest gtest_main)

    # Add test
    add_test(NAME ${TESTNAME} COMMAND ${TESTNAME})

endforeach(test)
