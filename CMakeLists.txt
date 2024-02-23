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

# enable features
include(FetchContent)
include(CheckIncludeFileCXX)
find_package(PkgConfig)

#------------------------------------------------------------------------------#
# Parameters for benchmarking
#------------------------------------------------------------------------------#

set(
  PMWCAS_BENCH_ELEMENT_NUM 
  "1000000" CACHE STRING 
  "The number of elements in a target array."
)

set(
  PMWCAS_BENCH_TARGET_NUM 
  "3" CACHE STRING 
  "The maximum number of target words of PMwCAS."
)

#------------------------------------------------------------------------------#
# Configure system libraries
#------------------------------------------------------------------------------#

# use gflags for CLI options
find_package(gflags REQUIRED)

# use PMDK
pkg_check_modules(LIBPMEM REQUIRED libpmem)
pkg_check_modules(LIBPMEMOBJ REQUIRED libpmemobj)

#------------------------------------------------------------------------------#
# Configure external libraries
#------------------------------------------------------------------------------#

FetchContent_Declare(
  cpp_utility
  GIT_REPOSITORY https://github.com/dbgroup-nagoya-u/cpp-utility.git
  GIT_TAG "c37418e13d8e258b9b76470a4958a974da37c093"
)
FetchContent_MakeAvailable(cpp_utility)

FetchContent_Declare(
  cpp_bench
  GIT_REPOSITORY https://github.com/dbgroup-nagoya-u/cpp-benchmark.git
  GIT_TAG "4b3aca13ecc4bf7bd2ea1930bebcba9083e71e2f"
)
FetchContent_MakeAvailable(cpp_bench)

set(MEMORY_MANAGER_USE_PERSISTENT_MEMORY "ON" CACHE STRING "" FORCE)
FetchContent_Declare(
  memory-manager
  GIT_REPOSITORY "https://github.com/dbgroup-nagoya-u/memory-manager.git"
  GIT_TAG "d9664115dcdc026983984b7a0dae3907fbf59d18"
)
FetchContent_MakeAvailable(memory-manager)

#------------------------------------------------------------------------------#
# Configure competitors
#------------------------------------------------------------------------------#

set(PMWCAS_CAPACITY "${PMWCAS_BENCH_TARGET_NUM}" CACHE STRING "" FORCE)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/pmwcas")

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/microsoft_pmwcas.cmake")

#------------------------------------------------------------------------------#
# Utility function for setting compile definitions
#------------------------------------------------------------------------------#

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
    PMWCAS_BENCH_ELEMENT_NUM=${PMWCAS_BENCH_ELEMENT_NUM}
    PMWCAS_BENCH_TARGET_NUM=${PMWCAS_BENCH_TARGET_NUM}
  )
  target_include_directories(${PMWCAS_BENCH_TARGET} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${LIBPMEM_INCLUDE_DIRS}"
    "${LIBPMEMOBJ_INCLUDE_DIRS}"
  )
  target_link_libraries(${PMWCAS_BENCH_TARGET} PRIVATE
    ${LIBPMEM_LIBRARIES}
    ${LIBPMEMOBJ_LIBRARIES}
    gflags
    dbgroup::cpp_utility
    dbgroup::cpp_bench
    dbgroup::memory_manager
    dbgroup::pmwcas
    microsoft::pmwcas
  )
endfunction()

#------------------------------------------------------------------------------#
# Build an array-based benchmark
#------------------------------------------------------------------------------#

add_executable(pmwcas_bench
  "${CMAKE_CURRENT_SOURCE_DIR}/src/pmwcas_bench.cpp"
)
PMWCAS_BENCH_ADD_EXECUTABLE("pmwcas_bench")

#------------------------------------------------------------------------------#
# Build a queue-based benchmark
#------------------------------------------------------------------------------#

# if(${PMWCAS_BENCH_TARGET_NUM} EQUAL 3)
#   add_executable(queue_bench
#     "${CMAKE_CURRENT_SOURCE_DIR}/src/queue_bench.cpp"
#   )
#   PMWCAS_BENCH_ADD_EXECUTABLE("queue_bench")
# endif()

#------------------------------------------------------------------------------#
# Build unit tests
#------------------------------------------------------------------------------#

option(PMWCAS_BENCH_BUILD_TESTS "Build unit tests" OFF)
if(${PMWCAS_BENCH_BUILD_TESTS})
  # enable CTest
  enable_testing()
  # include unit tests
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/test")
endif()
