cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
project(DNFC)
set (CMAKE_CXX_STANDARD 11)

# Must use GNUInstallDirs to install libraries into correct
# locations on all platforms.
include(GNUInstallDirs)
include(FetchContent)

# POSIX thread library
find_package(Threads REQUIRED)

# Google Tests
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.8.1
)
FetchContent_GetProperties(googletest)
if(NOT googletest_POPULATED)
  FetchContent_Populate(googletest)
  add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
endif()

# ThreadPool
FetchContent_Declare(
  threadpool
  GIT_REPOSITORY https://github.com/progschj/ThreadPool.git
)
FetchContent_GetProperties(threadpool)
if(NOT threadpool_POPULATED)
  FetchContent_Populate(threadpool)
  include_directories(${threadpool_SOURCE_DIR})
endif()

# Enable debug and include sub directory
enable_testing()
add_subdirectory(src)
