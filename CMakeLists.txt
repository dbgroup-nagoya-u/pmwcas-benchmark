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
# Parameters for benchmarking
#--------------------------------------------------------------------------------------#

set(PMWCAS_BENCH_ELEMENT_NUM "1000000" CACHE STRING "The number of elements in a target array.")
set(PMWCAS_BENCH_TARGET_NUM "2" CACHE STRING "The maximum number of target words of PMwCAS.")
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
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/external/memory-manager")
set(MEMORY_MANAGER_USE_PERSISTENT_MEMORY "ON" CACHE STRING "" FORCE)

#--------------------------------------------------------------------------------------#
# Configure Microsoft's PMwCAS if needed
#--------------------------------------------------------------------------------------#

option(PMWCAS_BENCH_USE_MICROSOFT_PMWCAS "Use microsoft/pmwcas as a PMwCAS implementation." OFF)
if(${PMWCAS_BENCH_USE_MICROSOFT_PMWCAS})
  # use PMDK for persistency
  set(PMEM_BACKEND "PMDK" CACHE STRING "" FORCE)

  # set PMwCAS capacity
  set(DESC_CAP "${PMWCAS_BENCH_TARGET_NUM}" CACHE STRING "" FORCE)

  # prevent building Google libraries and unit tests
  set(GOOGLE_FRAMEWORK OFF CACHE BOOL "" FORCE)
  set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
  set(BUILD_APPS OFF CACHE BOOL "" FORCE)

  add_subdirectory("${MICROSOFT_PMWCAS_SOURCE_DIR}")
endif()

#--------------------------------------------------------------------------------------#
# Build Benchmark
#--------------------------------------------------------------------------------------#

# build executable
add_executable(pmwcas_bench
  "${PMWCAS_BENCH_SOURCE_DIR}/src/array/pmwcas_bench.cpp"
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
  PMWCAS_BENCH_ELEMENT_NUM=${PMWCAS_BENCH_ELEMENT_NUM}
  PMWCAS_BENCH_TARGET_NUM=${PMWCAS_BENCH_TARGET_NUM}
  PMWCAS_BENCH_MAX_THREAD_NUM=${PMWCAS_BENCH_MAX_THREAD_NUM}
  $<$<BOOL:${PMWCAS_BENCH_USE_MICROSOFT_PMWCAS}>:PMWCAS_BENCH_USE_MICROSOFT_PMWCAS>
)
target_include_directories(pmwcas_bench PRIVATE
  "${PMWCAS_BENCH_SOURCE_DIR}/src"
  "${LIBPMEMOBJ++_INCLUDE_DIRS}"
)
target_link_libraries(pmwcas_bench PRIVATE
  ${LIBPMEMOBJ++_LIBRARIES}
  gflags
  cpp_utility
  cpp_bench
  dbgroup::memory_manager
)

if(${PMWCAS_BENCH_USE_MICROSOFT_PMWCAS})
  target_compile_definitions(pmwcas_bench PRIVATE
    PMEM
    PMDK
    DESC_CAP=${PMWCAS_BENCH_TARGET_NUM}
  )
  target_link_libraries(pmwcas_bench PRIVATE
    pmwcas_static
    rt
  )
  target_include_directories(pmwcas_bench PRIVATE
    "${MICROSOFT_PMWCAS_SOURCE_DIR}/"
    "${MICROSOFT_PMWCAS_SOURCE_DIR}/src"
    "${MICROSOFT_PMWCAS_SOURCE_DIR}/include"
  )
else()
  target_link_libraries(pmwcas_bench PRIVATE
    #pmwcas
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
