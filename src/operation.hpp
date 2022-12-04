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

#ifndef MWCAS_BENCHMARK_OPERATION_H
#define MWCAS_BENCHMARK_OPERATION_H

#include <algorithm>
#include <array>

#include "common.hpp"

class Operation
{
 public:
  /*################################################################################################
   * Public constructors and assignment operators
   *##############################################################################################*/

  constexpr Operation() : targets_{} {}

  constexpr Operation(const Operation &) = default;
  constexpr Operation &operator=(const Operation &obj) = default;
  constexpr Operation(Operation &&) = default;
  constexpr Operation &operator=(Operation &&) = default;

  /*################################################################################################
   * Public destructors
   *##############################################################################################*/

  ~Operation() = default;

  /*################################################################################################
   * Public getters/setters
   *##############################################################################################*/

  constexpr uint64_t *
  GetAddr(const size_t i) const
  {
    return targets_[i];
  }

  bool
  SetAddr(  //
      const size_t i,
      uint64_t *addr)
  {
    // check the target address has been already set
    const auto cur_end = targets_.begin() + i;
    if (std::find(targets_.begin(), cur_end, addr) != cur_end) return false;

    targets_[i] = addr;
    return true;
  }

  /*################################################################################################
   * Public utility functions
   *##############################################################################################*/

  /**
   * @brief Sort target addresses to linearize MwCAS operations.
   *
   */
  void
  SortTargets()
  {
    std::sort(targets_.begin(), targets_.end());
  }

 private:
  /*################################################################################################
   * Internal member variables
   *##############################################################################################*/

  /// target addresses of an MwCAS operation
  std::array<uint64_t *, kTargetNum> targets_;
};

#endif  // MWCAS_BENCHMARK_OPERATION_H
