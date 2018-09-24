# Define library. Only source files here!
project(dnfclib)

# Recursively get all source files
file(GLOB_RECURSE SOURCES 
    *.cpp
)

# Recursively get all header files
file(GLOB_RECURSE HEADERS
    *.hpp
)

# Recursively get all cpp files of the test
file(GLOB_RECURSE TEST_SOURCES 
    *_test.cpp
    *_test.hpp
)

# Remove tests from source files
foreach(test ${TEST_SOURCES})
    list(REMOVE_ITEM SOURCES ${test}) 
    list(REMOVE_ITEM HEADERS ${test})   
endforeach(test)

# Get path of include directory
foreach(source ${HEADERS})
    get_filename_component(dummy ${source} DIRECTORY)
    list(APPEND INCLUDE_DIRECTORIES ${dummy})
endforeach(source)

# Add source files to DNFC library and include directories
add_library(DNFC ${SOURCES} ${HEADERS})
target_include_directories(DNFC PRIVATE "$<BUILD_INTERFACE:${INCLUDE_DIRECTORIES}>")

# Scan for tests
foreach(test ${TEST_SOURCES})
    # Create test executable
    get_filename_component(TESTNAME ${test} NAME)
    string(REGEX MATCH "^(.*)\\.[^.]*$" dummy ${TESTNAME})
    set(TESTNAME ${CMAKE_MATCH_1})
    message(STATUS "Test found: " ${TESTNAME})
    add_executable(${TESTNAME} ${test})

    # Add DNFC library as depence
    target_link_libraries(${TESTNAME} 
                          DNFC
                          gtest gtest_main)

    # Add test
    add_test(${TESTNAME} ${TESTNAME})

endforeach(test)
