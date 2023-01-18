cmake_minimum_required(VERSION "3.8")

# decline in-source build
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "In-source builds are not allowed.")
endif()

# project information
project(
  pmwcas_bench
  VERSION "0.1.0"
  DESCRIPTION "A benchmark program for PMwCAS implementations."
  LANGUAGES "CXX"
)

# define source directories
set(MICROSOFT_PMWCAS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/microsoft-pmwcas")

#--------------------------------------------------------------------------------------#
# Parameters for benchmarking
#--------------------------------------------------------------------------------------#

set(PMWCAS_BENCH_ELEMENT_NUM "1000000" CACHE STRING "The number of elements in a target array.")
set(PMWCAS_BENCH_TARGET_NUM "3" CACHE STRING "The maximum number of target words of PMwCAS.")
set(PMWCAS_BENCH_MAX_THREAD_NUM "256" CACHE STRING "The maximum number of threads for benchmarking.")

#--------------------------------------------------------------------------------------#
# Configure PMDK
#--------------------------------------------------------------------------------------#

find_package(PkgConfig)
pkg_check_modules(LIBPMEMOBJ++ REQUIRED libpmemobj++)

#--------------------------------------------------------------------------------------#
# Configure external libraries
#--------------------------------------------------------------------------------------#

# use gflags for CLI options
find_package(gflags REQUIRED)

# our C++ utility
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/cpp-utility")

# our C++ benchmarker
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/cpp-benchmark")

# our GC
set(MEMORY_MANAGER_USE_PERSISTENT_MEMORY "ON" CACHE STRING "" FORCE)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/memory-manager")

set(PMWCAS_CAPACITY "${PMWCAS_BENCH_TARGET_NUM}" CACHE STRING "" FORCE)
set(PMWCAS_DESCRIPTOR_POOL_SIZE "${PMWCAS_BENCH_MAX_THREAD_NUM}" CACHE STRING "" FORCE)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/pmwcas")

#--------------------------------------------------------------------------------------#
# Configure Microsoft's PMwCAS
#--------------------------------------------------------------------------------------#

include("${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists_microsoft_pmwcas.txt")

#--------------------------------------------------------------------------------------#
# Utility function for setting compile definitions
#--------------------------------------------------------------------------------------#

# define function to add executable files
function(PMWCAS_BENCH_ADD_EXECUTABLE PMWCAS_BENCH_TARGET)
  target_compile_features(${PMWCAS_BENCH_TARGET} PRIVATE
    "cxx_std_17"
  )
  target_compile_options(${PMWCAS_BENCH_TARGET} PRIVATE
    -Wall
    -Wextra
    $<$<STREQUAL:${CMAKE_BUILD_TYPE},"Release">:"-O2 -march=native">
    $<$<STREQUAL:${CMAKE_BUILD_TYPE},"RelWithDebInfo">:"-g3 -Og -pg">
    $<$<STREQUAL:${CMAKE_BUILD_TYPE},"Debug">:"-g3 -O0 -pg">
  )
  target_compile_definitions(${PMWCAS_BENCH_TARGET} PRIVATE
    PMWCAS_BENCH_MAX_THREAD_NUM=${PMWCAS_BENCH_MAX_THREAD_NUM}
    PMWCAS_BENCH_ELEMENT_NUM=${PMWCAS_BENCH_ELEMENT_NUM}
    PMWCAS_BENCH_TARGET_NUM=${PMWCAS_BENCH_TARGET_NUM}
  )
  target_include_directories(${PMWCAS_BENCH_TARGET} PRIVATE
    "${PROJECT_SOURCE_DIR}/src"
    "${LIBPMEMOBJ++_INCLUDE_DIRS}"
  )
  target_link_libraries(${PMWCAS_BENCH_TARGET} PRIVATE
    ${LIBPMEMOBJ++_LIBRARIES}
    gflags
    cpp_utility
    cpp_bench
    dbgroup::memory_manager
    pmwcas::pmwcas
    microsoft_pmwcas
  )
endfunction()

#--------------------------------------------------------------------------------------#
# Build an array-based benchmark
#--------------------------------------------------------------------------------------#

add_executable(pmwcas_bench
  "${CMAKE_CURRENT_SOURCE_DIR}/src/pmwcas_bench.cpp"
)
PMWCAS_BENCH_ADD_EXECUTABLE("pmwcas_bench")

#--------------------------------------------------------------------------------------#
# Build a queue-based benchmark
#--------------------------------------------------------------------------------------#

if(${PMWCAS_BENCH_TARGET_NUM} EQUAL 3)
  add_executable(queue_bench
    "${CMAKE_CURRENT_SOURCE_DIR}/src/queue_bench.cpp"
  )
  PMWCAS_BENCH_ADD_EXECUTABLE("queue_bench")
endif()

#--------------------------------------------------------------------------------------#
# Build unit tests
#--------------------------------------------------------------------------------------#

option(PMWCAS_BENCH_BUILD_TESTS "Build unit tests" OFF)
if(${PMWCAS_BENCH_BUILD_TESTS})
  # enable CTest
  enable_testing()
  # include unit tests
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/test")
endif()
