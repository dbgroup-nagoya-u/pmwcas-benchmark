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

#ifndef PMWCAS_BENCHMARK_ARRAY_OPERATION_HPP
#define PMWCAS_BENCHMARK_ARRAY_OPERATION_HPP

// C++ standard libraries
#include <algorithm>
#include <array>

// local sources
#include "common.hpp"

class Operation
{
 public:
  /*####################################################################################
   * Public constructors and assignment operators
   *##################################################################################*/

  constexpr Operation() = default;

  constexpr Operation(const Operation &) = default;
  constexpr Operation &operator=(const Operation &obj) = default;
  constexpr Operation(Operation &&) = default;
  constexpr Operation &operator=(Operation &&) = default;

  /*####################################################################################
   * Public destructors
   *##################################################################################*/

  ~Operation() = default;

  /*####################################################################################
   * Public getters/setters
   *##################################################################################*/

  /**
   * @param i the index in PMwCAS operations.
   * @return the position of i-th target.
   */
  auto
  GetPosition(const size_t i) const  //
      -> size_t
  {
    return targets_[i];
  }

  /**
   * @brief Set the position of an element as i-th target.
   *
   * Note that this function check the uniqueness of positions for linearizability of
   * PMwCAS operations.
   *
   * @param i specify i-th target.
   * @param pos the position in an array.
   * @retval true if the position has been set.
   * @retval false otherwise.
   */
  auto
  SetPositionIfUnique(  //
      const size_t i,
      const size_t pos)  //
      -> bool
  {
    // check the target address has been already set
    const auto &cur_end = targets_.begin() + i;
    if (std::find(targets_.begin(), cur_end, pos) != cur_end) return false;

    targets_.at(i) = pos;
    return true;
  }

  /*####################################################################################
   * Public utility functions
   *##################################################################################*/

  /**
   * @brief Sort target addresses to linearize PMwCAS operations.
   *
   */
  void
  SortTargets()
  {
    std::sort(targets_.begin(), targets_.end());
  }

 private:
  /*####################################################################################
   * Internal member variables
   *##################################################################################*/

  /// target addresses of an MwCAS operation
  std::array<size_t, kTargetNum> targets_{};
};

#endif  // PMWCAS_BENCHMARK_ARRAY_OPERATION_HPP
