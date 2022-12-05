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
#include <string>

/*######################################################################################
 * Type aliases for competitors
 *####################################################################################*/

/// an alias for lock-based implementations.
using Lock = ::pmem::obj::mutex;

/*######################################################################################
 * Global constants and enums
 *####################################################################################*/

/// a file permission for pmemobj_pool.
#define CREATE_MODE_RW (S_IWUSR | S_IRUSR)

/// the number of elements in a target array.
constexpr size_t kElementNum = PMWCAS_BENCH_ELEMENT_NUM;

/// the maximum number of PMwCAS targets.
constexpr size_t kTargetNum = PMWCAS_BENCH_TARGET_NUM;

/// the layout name for benchmarking with arrays.
const std::string kArrayBenchLayout = "pmwcas_bench_with_array";

#endif  // PMWCAS_BENCHMARK_COMMON_HPP
