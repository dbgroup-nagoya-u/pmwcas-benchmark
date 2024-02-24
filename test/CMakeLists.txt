#------------------------------------------------------------------------------#
# Configure GoogleTest
#------------------------------------------------------------------------------#

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY "https://github.com/google/googletest.git"
  GIT_TAG "58d77fa8070e8cec2dc1ed015d66b454c8d78850" # v1.12.1
)
FetchContent_MakeAvailable(googletest)

#------------------------------------------------------------------------------#
# Configurations for unit tests
#------------------------------------------------------------------------------#

set(
  DBGROUP_TEST_THREAD_NUM
  "8" CACHE STRING
  "The number of worker threads for testing."
)

set(
  DBGROUP_TEST_TMP_PMEM_PATH
  "" CACHE STRING
  "The path to a durable storage."
)

#------------------------------------------------------------------------------#
# Build Unit Tests
#------------------------------------------------------------------------------#

# define function to add unit tests in the same format
function(DBGROUP_ADD_TEST DBGROUP_TEST_TARGET)
  add_executable(${DBGROUP_TEST_TARGET}
    "${CMAKE_CURRENT_SOURCE_DIR}/${DBGROUP_TEST_TARGET}.cpp"
  )
  target_compile_features(${DBGROUP_TEST_TARGET} PRIVATE
    "cxx_std_17"
  )
  target_compile_options(${DBGROUP_TEST_TARGET} PRIVATE
    -Wall
    -Wextra
    $<$<STREQUAL:${CMAKE_BUILD_TYPE},"Release">:"-O2 -march=native">
    $<$<STREQUAL:${CMAKE_BUILD_TYPE},"RelWithDebInfo">:"-g3 -Og -pg">
    $<$<STREQUAL:${CMAKE_BUILD_TYPE},"Debug">:"-g3 -O0 -pg">
  )
  target_compile_definitions(${DBGROUP_TEST_TARGET} PRIVATE
    DBGROUP_TEST_THREAD_NUM=${DBGROUP_TEST_THREAD_NUM}
    DBGROUP_TEST_TMP_PMEM_PATH=${DBGROUP_TEST_TMP_PMEM_PATH}
    PMWCAS_BENCH_MAX_TARGET_NUM=${PMWCAS_BENCH_MAX_TARGET_NUM}
  )
  target_include_directories(${DBGROUP_TEST_TARGET} PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
    "${LIBPMEM_INCLUDE_DIRS}"
    "${LIBPMEMOBJ_INCLUDE_DIRS}"
  )
  target_link_libraries(${DBGROUP_TEST_TARGET} PRIVATE
    ${LIBPMEM_LIBRARIES}
    ${LIBPMEMOBJ_LIBRARIES}
    dbgroup::cpp_utility
    dbgroup::pmwcas
    microsoft::pmwcas
    GTest::gtest_main
  )

  add_test(NAME ${DBGROUP_TEST_TARGET} COMMAND $<TARGET_FILE:${DBGROUP_TEST_TARGET}>)
endfunction()

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/array")
