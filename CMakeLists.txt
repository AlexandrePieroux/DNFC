cmake_minimum_required(VERSION 3.7 FATAL_ERROR)
project(DNFC)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Must use GNUInstallDirs to install libraries into correct
# locations on all platforms.
include(GNUInstallDirs)

# Google Tests
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Boost
find_package(Boost REQUIRED)
set(Boost_USE_STATIC_LIBS ON)
include_directories(${Boost_INCLUDE_DIR})

# Enable debug and include sub directory
enable_testing()
add_subdirectory(src)
