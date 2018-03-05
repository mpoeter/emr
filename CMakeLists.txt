cmake_minimum_required(VERSION 2.8)
project(benchmark)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME ON)

enable_testing()

find_package(Boost 1.61.0 COMPONENTS program_options REQUIRED)
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

set(EMR_FILES
        include/emr/detail/aligned_object.hpp
        include/emr/detail/allocation_tracker.hpp
        include/emr/detail/backoff.hpp
        include/emr/detail/concurrent_ptr.hpp
        include/emr/detail/deletable_object.hpp
        include/emr/detail/guard_ptr.hpp
        include/emr/detail/marked_ptr.hpp
        include/emr/detail/orphan.hpp
        include/emr/detail/perf_counter.hpp
        include/emr/detail/port.hpp
        include/emr/detail/thread_block_list.hpp
        include/emr/acquire_guard.hpp
        include/emr/debra.hpp
        include/emr/debra_impl.hpp
        include/emr/dummy.hpp
        include/emr/epoch_based.hpp
        include/emr/epoch_based_impl.hpp
        include/emr/hazard_pointer.hpp
        include/emr/hazard_pointer_impl.hpp
        include/emr/lock_free_ref_count.hpp
        include/emr/lock_free_ref_count_impl.hpp
        include/emr/new_epoch_based.hpp
        include/emr/new_epoch_based_impl.hpp
        include/emr/quiescent_state_based.hpp
        include/emr/quiescent_state_based_impl.hpp
        include/emr/stamp_it.hpp
        include/emr/stamp_it_impl.hpp
        include/debug_helper.hpp
        include/hash_map.hpp
        include/list.hpp
        include/queue.hpp)

set(BENCHMARK_FILES
        benchmarks/benchmark.hpp
        benchmarks/console_output_formatter.hpp
        benchmarks/csv_output_formatter.hpp
        benchmarks/guard_ptr_benchmark.hpp
        benchmarks/hash_map_benchmark.hpp
        benchmarks/list_benchmark.hpp
        benchmarks/main.cpp
        benchmarks/output_formatter.hpp
        benchmarks/process_memory.hpp
        benchmarks/process_memory.cpp
        benchmarks/queue_benchmark.hpp
        benchmarks/test_execution.cpp
        benchmarks/test_execution.hpp)

set(TEST_FILES
        test/concurrent_ptr_test.cpp
        test/epoch_based_test.cpp
        test/hash_map_test.cpp
        test/hazard_pointer_test.cpp
        test/list_test.cpp
        test/lock_free_ref_count_test.cpp
        test/main.cpp
        test/marked_ptr_test.cpp
        test/new_epoch_based_test.cpp
        test/queue_test.cpp
        test/quiescent_state_based_test.cpp
        test/stamp_it_test.cpp
        test/debra_test.cpp)

include_directories(include)
include_directories(${Boost_INCLUDE_DIR})
include_directories(${GTEST_INCLUDE_DIRS})

link_directories(${Boost_LIBRARY_DIR})

add_definitions(-DBOOST_NO_AUTO_PTR)

add_executable(benchmark ${BENCHMARK_FILES} ${EMR_FILES})
add_executable(gtest ${TEST_FILES} ${EMR_FILES})

target_link_libraries(gtest ${GTEST_BOTH_LIBRARIES})

target_link_libraries(benchmark ${Boost_LIBRARIES})
if(CMAKE_THREAD_LIBS_INIT)
    target_link_libraries(benchmark "${CMAKE_THREAD_LIBS_INIT}")
    target_link_libraries(gtest "${CMAKE_THREAD_LIBS_INIT}")
endif()

add_test(AllTests gtest)