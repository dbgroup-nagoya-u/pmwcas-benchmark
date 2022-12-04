cmake_minimum_required(VERSION "3.8")

# decline in-source build
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "In-source builds are not allowed.")
endif()

# project information
project(
  mwcas_bench
  VERSION "0.1.0"
  DESCRIPTION "A benchmark program for MwCAS implementations."
  LANGUAGES "CXX"
)

# define source directories
set(MWCAS_BENCH_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(PMWCAS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/pmwcas")

#--------------------------------------------------------------------------------------#
# Set Compile Options
#--------------------------------------------------------------------------------------#

set(
  MWCAS_BENCH_TARGET_NUM
  "2"
  CACHE STRING
  "The maximum number of target words of MwCAS."
)

#--------------------------------------------------------------------------------------#
# Configure external libraries
#--------------------------------------------------------------------------------------#

# use gflags for CLI options
find_package(gflags REQUIRED)

# our C++ utility
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/cpp-utility")

# our C++ benchmarker
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/cpp-benchmark")

#--------------------------------------------------------------------------------------#
# Configure MwCAS
#--------------------------------------------------------------------------------------#

# set MwCAS capacity
set(MWCAS_CAPACITY "${MWCAS_BENCH_TARGET_NUM}" CACHE STRING "" FORCE)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/mwcas")

#--------------------------------------------------------------------------------------#
# Configure PMwCAS
#--------------------------------------------------------------------------------------#

# build volatile PMwCAS
set(PMEM_BACKEND "Volatile" CACHE STRING "" FORCE)

# set MwCAS capacity
set(DESC_CAP "${MWCAS_BENCH_TARGET_NUM}" CACHE STRING "" FORCE)

# prevent building Google libraries
set(GOOGLE_FRAMEWORK OFF CACHE BOOL "" FORCE)

# prevent building unit tests
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_APPS OFF CACHE BOOL "" FORCE)

add_subdirectory("${PMWCAS_SOURCE_DIR}")

#--------------------------------------------------------------------------------------#
# Configure AOPT
#--------------------------------------------------------------------------------------#

set(MWCAS_AOPT_MWCAS_CAPACITY "${MWCAS_BENCH_TARGET_NUM}" CACHE STRING "" FORCE)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/mwcas-aopt")

#--------------------------------------------------------------------------------------#
# Build Benchmark
#--------------------------------------------------------------------------------------#

# build executable
add_executable(mwcas_bench
  "${MWCAS_BENCH_SOURCE_DIR}/src/mwcas_bench.cpp"
)
target_compile_features(mwcas_bench PRIVATE
  "cxx_std_17"
)
target_compile_options(mwcas_bench PRIVATE
  -Wall
  -Wextra
  $<$<STREQUAL:${CMAKE_BUILD_TYPE},"Release">:"-O2 -march=native">
  $<$<STREQUAL:${CMAKE_BUILD_TYPE},"RelWithDebInfo">:"-g3 -Og -pg">
  $<$<STREQUAL:${CMAKE_BUILD_TYPE},"Debug">:"-g3 -O0 -pg">
)
target_compile_definitions(mwcas_bench PRIVATE
  MWCAS_BENCH_TARGET_NUM=${MWCAS_BENCH_TARGET_NUM}
)
target_include_directories(mwcas_bench PRIVATE
  "${PMWCAS_SOURCE_DIR}/"
  "${PMWCAS_SOURCE_DIR}/src"
  "${PMWCAS_SOURCE_DIR}/include"
)
target_link_libraries(mwcas_bench PRIVATE
  mwcas
  pmwcas_static
  mwcas_aopt
  rt
  gflags
  cpp_utility
  cpp_bench
)

option(MWCAS_BENCH_OVERRIDE_JEMALLOC "Override entire memory allocation with jemalloc" OFF)
if(${MWCAS_BENCH_OVERRIDE_JEMALLOC})
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(JEMALLOC REQUIRED IMPORTED_TARGET jemalloc)

  target_include_directories(mwcas_bench PRIVATE
    ${JEMALLOC_INCLUDE_DIRS}
  )
  target_link_libraries(mwcas_bench PRIVATE
    PkgConfig::JEMALLOC
  )
endif()

#--------------------------------------------------------------------------------------#
# Build unit tests
#--------------------------------------------------------------------------------------#

option(MWCAS_BENCH_BUILD_TESTS "Build unit tests" OFF)
if(${MWCAS_BENCH_BUILD_TESTS})
  # enable CTest
  enable_testing()
  # include unit tests
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/test")
endif()
