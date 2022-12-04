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
set(PMWCAS_BENCH_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(MICROSOFT_PMWCAS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/microsoft-pmwcas")

#--------------------------------------------------------------------------------------#
# Set Compile Options
#--------------------------------------------------------------------------------------#

set(
  PMWCAS_BENCH_TARGET_NUM
  "2"
  CACHE STRING
  "The maximum number of target words of PMwCAS."
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
# Configure Microsoft's PMwCAS
#--------------------------------------------------------------------------------------#

# # build volatile PMwCAS
# set(PMEM_BACKEND "Volatile" CACHE STRING "" FORCE)

# # set MwCAS capacity
# set(DESC_CAP "${PMWCAS_BENCH_TARGET_NUM}" CACHE STRING "" FORCE)

# # prevent building Google libraries
# set(GOOGLE_FRAMEWORK OFF CACHE BOOL "" FORCE)

# # prevent building unit tests
# set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
# set(BUILD_APPS OFF CACHE BOOL "" FORCE)

# add_subdirectory("${PPMWCAS_SOURCE_DIR}")

#--------------------------------------------------------------------------------------#
# Build Benchmark
#--------------------------------------------------------------------------------------#

# build executable
add_executable(pmwcas_bench
  "${PMWCAS_BENCH_SOURCE_DIR}/src/pmwcas_bench.cpp"
)
target_compile_features(pmwcas_bench PRIVATE
  "cxx_std_17"
)
target_compile_options(pmwcas_bench PRIVATE
  -Wall
  -Wextra
  $<$<STREQUAL:${CMAKE_BUILD_TYPE},"Release">:"-O2 -march=native">
  $<$<STREQUAL:${CMAKE_BUILD_TYPE},"RelWithDebInfo">:"-g3 -Og -pg">
  $<$<STREQUAL:${CMAKE_BUILD_TYPE},"Debug">:"-g3 -O0 -pg">
)
target_compile_definitions(pmwcas_bench PRIVATE
  PMWCAS_BENCH_TARGET_NUM=${PMWCAS_BENCH_TARGET_NUM}
)
# target_include_directories(pmwcas_bench PRIVATE
#   "${MICROSOFT_PMWCAS_SOURCE_DIR}/"
#   "${MICROSOFT_PMWCAS_SOURCE_DIR}/src"
#   "${MICROSOFT_PMWCAS_SOURCE_DIR}/include"
# )
target_link_libraries(pmwcas_bench PRIVATE
  # pmwcas
  # pmwcas_static
  # rt
  gflags
  cpp_utility
  cpp_bench
)

option(PMWCAS_BENCH_OVERRIDE_JEMALLOC "Override entire memory allocation with jemalloc" OFF)
if(${PMWCAS_BENCH_OVERRIDE_JEMALLOC})
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(JEMALLOC REQUIRED IMPORTED_TARGET jemalloc)

  target_include_directories(pmwcas_bench PRIVATE
    ${JEMALLOC_INCLUDE_DIRS}
  )
  target_link_libraries(pmwcas_bench PRIVATE
    PkgConfig::JEMALLOC
  )
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
