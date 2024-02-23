# configure libraries
find_package(Threads)
find_package(PkgConfig)
pkg_check_modules(LIBPMEMOBJ REQUIRED libpmemobj)

# prepare source files
FetchContent_Declare(
  microsoft_pmwcas
  GIT_REPOSITORY "https://github.com/microsoft/pmwcas.git"
  GIT_TAG "3c50dec9cfbe31e3c4b02ef7e0ffd9f0a210adc3" # latest at Feb. 23, 2024
)
FetchContent_Populate(microsoft_pmwcas)
set(MICROSOFT_PMWCAS_SOURCES
  ${microsoft_pmwcas_SOURCE_DIR}/src/common/allocator_internal.cc
  ${microsoft_pmwcas_SOURCE_DIR}/src/common/pmwcas_internal.cc
  ${microsoft_pmwcas_SOURCE_DIR}/src/common/environment_internal.cc
  ${microsoft_pmwcas_SOURCE_DIR}/src/common/epoch.cc
  ${microsoft_pmwcas_SOURCE_DIR}/src/environment/environment.cc
  ${microsoft_pmwcas_SOURCE_DIR}/src/environment/environment_linux.cc
  ${microsoft_pmwcas_SOURCE_DIR}/src/mwcas/mwcas.cc
  ${microsoft_pmwcas_SOURCE_DIR}/src/util/nvram.cc
  ${microsoft_pmwcas_SOURCE_DIR}/src/util/status.cc
)

# fix target addresses to be flushed
set(MICROSOFT_PMWCAS_MWCAS_H "${microsoft_pmwcas_SOURCE_DIR}/src/mwcas/mwcas.h")
execute_process(
  COMMAND bash "-c" "sed -i '172 s/&address_/address_/' ${MICROSOFT_PMWCAS_MWCAS_H}"
)

# build library
add_library(microsoft_pmwcas STATIC ${MICROSOFT_PMWCAS_SOURCES})
add_library(microsoft::pmwcas ALIAS microsoft_pmwcas)
target_compile_features(microsoft_pmwcas PUBLIC
  "cxx_std_11"
)
target_include_directories(microsoft_pmwcas PUBLIC
  "${microsoft_pmwcas_SOURCE_DIR}/"
  "${microsoft_pmwcas_SOURCE_DIR}/src"
  "${microsoft_pmwcas_SOURCE_DIR}/include"
)
target_include_directories(microsoft_pmwcas PRIVATE
  "${LIBPMEMOBJ_INCLUDE_DIRS}"
)
target_compile_definitions(microsoft_pmwcas PUBLIC
  DESC_CAP=${PMWCAS_BENCH_TARGET_NUM}
  PMEM
  PMDK
)
target_link_libraries(microsoft_pmwcas PRIVATE
  Threads::Threads
  rt
  numa
  ${LIBPMEMOBJ_LIBRARIES}
)
