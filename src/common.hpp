/*
 * Copyright 2022 Database Group, Nagoya University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PMWCAS_BENCHMARK_COMMON_HPP
#define PMWCAS_BENCHMARK_COMMON_HPP

// system headers
#include <libpmemobj++/mutex.hpp>

// C++ standard libraries
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>

// external sources
#ifndef PMWCAS_BENCH_USE_MICROSOFT_PMWCAS
// #include "pmwcas/pmwcas_descriptor_pool.hpp"
#else
#include "mwcas/mwcas.h"
#endif

/*######################################################################################
 * Type aliases for competitors
 *####################################################################################*/

/// an alias for lock based implementations.
using Lock = ::pmem::obj::mutex;

#ifndef PMWCAS_BENCH_USE_MICROSOFT_PMWCAS
/// an alias for our PMwCAS based implementations.
// using PMwCAS = ::dbgroup::atomic::pmwcas::PMwCASDescriptorPool;
#else
/// an alias for microsoft/pmwcas based implementations.
using PMwCAS = ::pmwcas::DescriptorPool;
#endif

/*######################################################################################
 * Global constants and enums
 *####################################################################################*/

/// a file permission for pmemobj_pool.
#define CREATE_MODE_RW (S_IWUSR | S_IRUSR)

/// the number of elements in a target array.
constexpr size_t kElementNum = PMWCAS_BENCH_ELEMENT_NUM;

/// the maximum number of PMwCAS targets.
constexpr size_t kTargetNum = PMWCAS_BENCH_TARGET_NUM;

/// @brief The maximum number of threads for benchmarking.
constexpr size_t kMaxThreadNum = PMWCAS_BENCH_MAX_THREAD_NUM;

constexpr size_t kGCInterval = 1E5;

constexpr size_t kThreadNum = 1;

constexpr size_t kNullPtr = 0;

/// the layout name for benchmarking with arrays.
const std::string kArrayBenchLayout = "array";

#ifndef PMWCAS_BENCH_USE_MICROSOFT_PMWCAS
/// the layout name for the pool of PMwCAS descriptors.
const std::string kPMwCASLayout = "pmwcas";
#else
/// the layout name for the pool of PMwCAS descriptors.
const std::string kPMwCASLayout = "microsoft_pmwcas";
#endif

/*######################################################################################
 * Global utilities
 *####################################################################################*/

/**
 * @param pmem_dir_str the path to persistent memory.
 * @param layout the name of a layout for pmemobj_pool.
 * @return the file path for a given layout.
 */
auto
GetPath(  //
    const std::string &pmem_dir_str,
    const std::string &layout)  //
    -> std::string
{
  std::filesystem::path pmem_dir_path{pmem_dir_str};
  return pmem_dir_path.append(layout).native();
}

#endif  // PMWCAS_BENCHMARK_COMMON_HPP
