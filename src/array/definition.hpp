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

#ifndef PMWCAS_BENCHMARK_ARRAY_COMMON_HPP
#define PMWCAS_BENCHMARK_ARRAY_COMMON_HPP

// external libraries
#include <libpmemobj++/mutex.hpp>

#ifndef PMWCAS_BENCH_USE_MICROSOFT_PMWCAS
// #include "pmwcas/pmwcas_descriptor_pool.hpp"
#else
#include "mwcas/mwcas.h"
#include "pmwcas.h"
#endif

// local sources
#include "common.hpp"

/*######################################################################################
 * Global constants and enums
 *####################################################################################*/

/// the number of elements in a target array.
constexpr size_t kElementNum = PMWCAS_BENCH_ELEMENT_NUM;

/// the maximum number of PMwCAS targets.
constexpr size_t kTargetNum = PMWCAS_BENCH_TARGET_NUM;

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

#endif  // PMWCAS_BENCHMARK_ARRAY_COMMON_HPP
