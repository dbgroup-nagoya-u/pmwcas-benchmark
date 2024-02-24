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

#ifndef PMWCAS_BENCHMARK_COMPETITOR_HPP
#define PMWCAS_BENCHMARK_COMPETITOR_HPP

// our PMwCAS
#include "pmwcas/descriptor_pool.hpp"

// microsoft/pmwcas
#include "mwcas/mwcas.h"
#include "pmwcas.h"

/*##############################################################################
 * Type aliases for competitors
 *############################################################################*/

/// @brief An alias for our PMwCAS.
using PMwCAS = ::dbgroup::atomic::pmwcas::DescriptorPool;

/// @brief An alias for microsoft/pmwcas.
using MicrosoftPMwCAS = ::pmwcas::DescriptorPool;

/// @brief An alias for software PCAS.
using PCAS = void;

#endif  // PMWCAS_BENCHMARK_COMPETITOR_HPP
