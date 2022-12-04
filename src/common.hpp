/*
 * Copyright 2021 Database Group, Nagoya University
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

#ifndef MWCAS_BENCHMARK_COMMON_H
#define MWCAS_BENCHMARK_COMMON_H

#include <atomic>  // sngle CAS implementation
#include <cassert>
#include <cstddef>
#include <cstdint>

#include "aopt/aopt_descriptor.hpp"    // AOPT implementation
#include "mwcas/mwcas.h"               // PMwCAS implementation
#include "mwcas/mwcas_descriptor.hpp"  // our MwCAS without GC implementation

/*##################################################################################################
 * Global type aliases
 *################################################################################################*/

using MwCAS = ::dbgroup::atomic::mwcas::MwCASDescriptor;
using PMwCAS = ::pmwcas::DescriptorPool;
using AOPT = ::dbgroup::atomic::aopt::AOPTDescriptor;
using SingleCAS = ::std::atomic_size_t;

/*##################################################################################################
 * Global constants and enums
 *################################################################################################*/

/// the maximum number of MwCAS targets
constexpr size_t kTargetNum = MWCAS_BENCH_TARGET_NUM;

#endif  // MWCAS_BENCHMARK_COMMON_H
