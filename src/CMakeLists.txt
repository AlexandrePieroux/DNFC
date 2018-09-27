# Define library. Only source files here!
project(dnfclib)

# Recursively get all source files
file(GLOB_RECURSE SOURCES 
    "*.cpp"
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

# Add source files to DNFC library and include directories
add_library(DNFC SHARED ${SOURCES})

# Scan for tests
foreach(test ${TEST_SOURCES})
    # Get test name
    get_filename_component(TESTNAME ${test} NAME)
    string(REGEX MATCH "^(.*)\\.[^.]*$" dummy ${TESTNAME})
    set(TESTNAME ${CMAKE_MATCH_1})
    message(STATUS "Test found: " ${TESTNAME})

    # Create test executable
    add_executable(${TESTNAME} ${test})

    # Add DNFC library as depence
    target_link_libraries(${TESTNAME} 
                          DNFC
                          gtest gtest_main)

    # Add test
    add_test(NAME ${TESTNAME} COMMAND ${TESTNAME})

endforeach(test)
