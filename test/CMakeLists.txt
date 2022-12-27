#--------------------------------------------------------------------------------------#
# Configure GoogleTest
#--------------------------------------------------------------------------------------#

configure_file(CMakeLists-googletest.txt.in googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
                 ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
                 EXCLUDE_FROM_ALL)

#--------------------------------------------------------------------------------------#
# Build Unit Tests
#--------------------------------------------------------------------------------------#

  # define function to add unit tests in the same format
function(ADD_PMWCAS_BENCH_TEST PMWCAS_BENCH_TEST_TARGET)
  add_executable(${PMWCAS_BENCH_TEST_TARGET}
    "${CMAKE_CURRENT_SOURCE_DIR}/${PMWCAS_BENCH_TEST_TARGET}.cpp"
  )
  target_compile_features(${PMWCAS_BENCH_TEST_TARGET} PRIVATE
    "cxx_std_17"
  )
  target_compile_options(${PMWCAS_BENCH_TEST_TARGET} PRIVATE
    -Wall
    -Wextra
    $<$<STREQUAL:${CMAKE_BUILD_TYPE},"Release">:"-O2 -march=native">
    $<$<STREQUAL:${CMAKE_BUILD_TYPE},"RelWithDebInfo">:"-g3 -Og -pg">
    $<$<STREQUAL:${CMAKE_BUILD_TYPE},"Debug">:"-g3 -O0 -pg">
  )
  target_compile_definitions(${PMWCAS_BENCH_TEST_TARGET} PRIVATE
    PMWCAS_BENCH_ELEMENT_NUM=${PMWCAS_BENCH_ELEMENT_NUM}
    PMWCAS_BENCH_TARGET_NUM=${PMWCAS_BENCH_TARGET_NUM}
    PMWCAS_BENCH_MAX_THREAD_NUM=${PMWCAS_BENCH_MAX_THREAD_NUM}
    $<$<BOOL:${PMWCAS_BENCH_USE_MICROSOFT_PMWCAS}>:PMWCAS_BENCH_USE_MICROSOFT_PMWCAS>
  )
  target_include_directories(${PMWCAS_BENCH_TEST_TARGET} PRIVATE
    "${PMWCAS_BENCH_SOURCE_DIR}/src"
    "${LIBPMEMOBJ++_INCLUDE_DIRS}"
  )
  target_link_libraries(${PMWCAS_BENCH_TEST_TARGET} PRIVATE
    ${LIBPMEMOBJ++_LIBRARIES}
    cpp_utility
    dbgroup::memory_manager
    gtest_main
  )

  if(${PMWCAS_BENCH_USE_MICROSOFT_PMWCAS})
    target_compile_definitions(${PMWCAS_BENCH_TEST_TARGET} PRIVATE
      PMEM
      PMDK
      DESC_CAP=${PMWCAS_BENCH_TARGET_NUM}
    )
    target_link_libraries(${PMWCAS_BENCH_TEST_TARGET} PRIVATE
      pmwcas_static
      rt
    )
    target_include_directories(${PMWCAS_BENCH_TEST_TARGET} PRIVATE
      "${MICROSOFT_PMWCAS_SOURCE_DIR}/"
      "${MICROSOFT_PMWCAS_SOURCE_DIR}/src"
      "${MICROSOFT_PMWCAS_SOURCE_DIR}/include"
    )
  else()
    target_link_libraries(${PMWCAS_BENCH_TEST_TARGET} PRIVATE
      #pmwcas
    )
  endif()

  add_test(NAME ${PMWCAS_BENCH_TEST_TARGET} COMMAND $<TARGET_FILE:${PMWCAS_BENCH_TEST_TARGET}>)
endfunction()

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/array")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/queue")
