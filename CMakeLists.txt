cmake_minimum_required (VERSION 2.8.2)
project (DNFC)

# Libraries
set (MEMORY_MANAGEMENT
		 libs/memory_management/memory_management.c
		 libs/memory_management/memory_management.h)

set (HAZARD_POINTER
       libs/SMR/hazard_pointer.c
       libs/SMR/hazard_pointer.h)

set (BYTE_STREAM
       libs/byte_stream/byte_stream.c
       libs/byte_stream/byte_stream.h)

set (LINKED_LIST
		 libs/linked_list/linked_list.c
		 libs/linked_list/linked_list.h)

set (HASH_TABLE
		 libs/hash_table/FNV-1a.c
		 libs/hash_table/FNV-1a.h
		 libs/hash_table/murmur3.c
		 libs/hash_table/murmur3.h
		 libs/hash_table/hash_table.c
		 libs/hash_table/hash_table.h)

set (QUEUE
       libs/queue/queue.c
       libs/queue/queue.h)

set (THREAD_POOL
		 libs/thread_pool/job_queue.c
		 libs/thread_pool/job_queue.h
		 libs/thread_pool/thread_pool.c
		 libs/thread_pool/thread_pool.h)


# classifiers
set (CLASSIFIERS_RULES
		 src/classifiers/classifier_rule/classifier_rule.h
		 src/classifiers/classifier_rule/classifier_field.h)

# dynamic classifiers
set (FLOW_TABLE
		 src/classifiers/dynamic_classifiers/flow_table/flow_table.c
		 src/classifiers/dynamic_classifiers/flow_table/flow_table.h)

set (DYNAMIC_CLASSIFIERS
		 src/classifiers/dynamic_classifiers/dynamic_classifiers.h
		 ${CLASSIFIERS_RULES}
		 ${FLOW_TABLE})


# static classifiers
set (HYPERCUTS
		 src/classifiers/static_classifiers/hypercuts/hypercuts.c
		 src/classifiers/static_classifiers/hypercuts/hypercuts.h)

set (STATIC_CLASSIFIERS
		 src/classifiers/static_classifiers/static_classifiers.h
		 ${CLASSIFIERS_RULES}
		 ${HYPERCUTS})

# main structure
set (DNFC
		 src/DNFC.c
	 	 src/DNFC.h)


# add the Libraries
add_library (memory_management ${MEMORY_MANAGEMENT})
add_library (SMR ${HAZARD_POINTER})
add_library (byte_stream ${BYTE_STREAM})
add_library (linked_list ${LINKED_LIST})
add_library (hash_table ${HASH_TABLE})
add_library (queue ${QUEUE})
add_library (thread_pool ${THREAD_POOL})
add_library (dynamic_classifiers ${DYNAMIC_CLASSIFIERS})
add_library (static_classifiers ${STATIC_CLASSIFIERS})


# add the executable
add_executable (DNFC ${DNFC})


# link Libraries
target_link_libraries (SMR
                       memory_management)
target_link_libraries (linked_list
                       SMR
                       byte_stream)
target_link_libraries (hash_table
                       linked_list
                       byte_stream)
target_link_libraries (thread_pool
                       memory_management)
target_link_libraries (dynamic_classifiers 
						     hash_table
                       byte_stream)
target_link_libraries (queue
                       SMR)
target_link_libraries (static_classifiers 
                       hash_table
                       byte_stream)
target_link_libraries (DNFC
                       thread_pool
                       dynamic_classifiers
                       static_classifiers
                       queue)

# Tests
# Download and unpack googletest at configure time
configure_file (CMakeLists.txt.in googletest-download/CMakeLists.txt)
execute_process (COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download)
if (result)
  message (FATAL_ERROR "CMake step for googletest failed: ${result}")
endif ()
execute_process (COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download)
if (result)
  message (FATAL_ERROR "Build step for googletest failed: ${result}")
endif ()

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set (gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory (${CMAKE_BINARY_DIR}/googletest-src
                  ${CMAKE_BINARY_DIR}/googletest-build)

# The gtest/gtest_main targets carry header search path
# dependencies automatically when using CMake 2.8.11 or
# later. Otherwise we have to add them here ourselves.
if (CMAKE_VERSION VERSION_LESS 2.8.11)
  include_directories ("${gtest_SOURCE_DIR}/include")
endif ()


set (LINKED_LIST_TEST tests/linked_list_test.cpp)
set (HASH_TABLE_TEST tests/hash_table_test.cpp)
set (HYPERCUTS_TEST tests/hypercuts_test.cpp)
set (FLOW_TABLE_TEST tests/flow_table_test.cpp)
set (QUEUE_TEST tests/queue_test.cpp)
set (DNFC_TEST tests/DNFC_test.cpp)

add_executable (linked_list_test ${LINKED_LIST_TEST})
add_executable (hash_table_test ${HASH_TABLE_TEST})
add_executable (hypercuts_test ${HYPERCUTS_TEST})
add_executable (flow_table_test ${FLOW_TABLE_TEST})
add_executable (queue_test ${QUEUE_TEST})
add_executable (DNFC_test ${DNFC_TEST})

target_link_libraries (linked_list_test
                        linked_list
                        byte_stream
                        thread_pool
                        gtest gtest_main)
target_link_libraries (hash_table_test
                        hash_table
                        byte_stream
                        thread_pool
                        gtest gtest_main)
target_link_libraries (hypercuts_test
                        static_classifiers
                        byte_stream
                        gtest gtest_main)
target_link_libraries (flow_table_test
                        dynamic_classifiers
                        byte_stream
                        thread_pool
                        gtest gtest_main)
target_link_libraries (queue_test
                        queue
                        thread_pool
                        gtest gtest_main)
target_link_libraries (DNFC_test
								hash_table
								thread_pool
								dynamic_classifiers
								static_classifiers
								gtest gtest_main)

add_test(NAME linked_list_test COMMAND linked_list_test)
add_test(NAME hash_table_test COMMAND hash_table_test)
add_test(NAME hypercuts_test COMMAND hypercuts_test)
add_test(NAME flow_table_test COMMAND flow_table_test)
add_test(NAME queue_test COMMAND queue_test)
add_test(NAME DNFC_test COMMAND DNFC_test)
