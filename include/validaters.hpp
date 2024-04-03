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

#ifndef PMWCAS_BENCHMARK_CLO_VALIDATORS_HPP
#define PMWCAS_BENCHMARK_CLO_VALIDATORS_HPP

// C++ standard libraries
#include <iostream>
#include <stdexcept>
#include <string>

/*##############################################################################
 * Validators for gflags
 *############################################################################*/

template <class Number>
static auto
ValidatePositiveVal(  //
    const char *flagname,
    const Number value)  //
    -> bool
{
  if (value >= 0) return true;

  std::cerr << "A value must be positive for " << flagname << "\n";
  return false;
}

template <class Number>
static auto
ValidateNonZero(  //
    const char *flagname,
    const Number value)  //
    -> bool
{
  if (value != 0) return true;

  std::cerr << "A value must be not zero for " << flagname << "\n";
  return false;
}

template <class UInt>
static auto
ValidateBlockSize(  //
    const char *flagname,
    const UInt value)  //
    -> bool
{
  if (value < 8) {
    std::cerr << "A value is too small: " << flagname << "\n";
    return false;
  }
  if (value & (value - 1)) {
    std::cerr << "A value must be the exponential in two: " << flagname << "\n";
    return false;
  }
  return true;
}

static auto
ValidateRandomSeed(  //
    [[maybe_unused]] const char *flagname,
    const std::string &seed)  //
    -> bool
{
  if (seed.empty()) return true;

  try {
    std::stoul(seed);
  } catch (const std::invalid_argument &) {
    std::cerr << "A random seed must be unsigned integers\n";
    return false;
  }
  return true;
}

#endif  // PMWCAS_BENCHMARK_CLO_VALIDATORS_HPP
