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
  PMWCAS_BENCH_MAX_TARGET_NUM
  "8" CACHE STRING
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
  GIT_TAG "4a23e598ae5ede15b9c03c860ac156f19ba8112a"
)
FetchContent_MakeAvailable(cpp_utility)

FetchContent_Declare(
  cpp_bench
  GIT_REPOSITORY https://github.com/dbgroup-nagoya-u/cpp-benchmark.git
  GIT_TAG "46539c221b1716ab54ffc2af0ddd218851f917e1"
)
FetchContent_MakeAvailable(cpp_bench)

#------------------------------------------------------------------------------#
# Configure competitors
#------------------------------------------------------------------------------#

set(PMEM_ATOMIC_PMWCAS_CAPACITY "${PMWCAS_BENCH_MAX_TARGET_NUM}" CACHE STRING "" FORCE)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/pmwcas")

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/microsoft_pmwcas.cmake")

#------------------------------------------------------------------------------#
# Build targets
#------------------------------------------------------------------------------#

add_executable(${PROJECT_NAME}
  "${CMAKE_CURRENT_SOURCE_DIR}/src/${PROJECT_NAME}.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/pmwcas_target.cpp"
)
target_compile_features(${PROJECT_NAME} PRIVATE
  "cxx_std_17"
)
target_compile_options(${PROJECT_NAME} PRIVATE
  -Wall
  -Wextra
  $<$<STREQUAL:${CMAKE_BUILD_TYPE},"Release">:"-O2 -march=native">
  $<$<STREQUAL:${CMAKE_BUILD_TYPE},"RelWithDebInfo">:"-g3 -Og -pg">
  $<$<STREQUAL:${CMAKE_BUILD_TYPE},"Debug">:"-g3 -O0 -pg">
)
target_include_directories(${PROJECT_NAME} PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${LIBPMEM_INCLUDE_DIRS}"
  "${LIBPMEMOBJ_INCLUDE_DIRS}"
)
target_link_libraries(${PROJECT_NAME} PRIVATE
  ${LIBPMEM_LIBRARIES}
  ${LIBPMEMOBJ_LIBRARIES}
  gflags
  dbgroup::cpp_utility
  dbgroup::cpp_bench
  dbgroup::pmem_atomic
  microsoft::pmwcas
)

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
