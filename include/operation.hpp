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
#include <cstddef>
#include <vector>

class Operation
{
 public:
  /*############################################################################
   * Public constructors and assignment operators
   *##########################################################################*/

  constexpr Operation() = default;

  Operation(const Operation &) = default;
  Operation(Operation &&) = default;

  Operation &operator=(const Operation &obj) = default;
  Operation &operator=(Operation &&) = default;

  /*############################################################################
   * Public destructors
   *##########################################################################*/

  ~Operation() = default;

  /*############################################################################
   * Public getters/setters
   *##########################################################################*/

  /**
   * @return The target positions in an array.
   */
  constexpr auto
  GetPositions() const  //
      -> const std::vector<size_t> &
  {
    return targets_;
  }

  /**
   * @brief Set the position of an element as i-th target.
   *
   * @param pos The position in an array.
   * @retval true if the position has been set.
   * @retval false otherwise.
   * @note This function checks the uniqueness of given positions for
   * guaranteeing linearizability of PMwCAS operations.
   */
  auto
  SetPositionIfUnique(   //
      const size_t pos)  //
      -> bool
  {
    // check the target address has been already set
    const auto &cur_end = targets_.end();
    if (std::find(targets_.begin(), cur_end, pos) != cur_end) return false;

    targets_.emplace_back(pos);
    return true;
  }

  /*############################################################################
   * Public utility functions
   *##########################################################################*/

  /**
   * @brief Sort target positions to linearize PMwCAS operations.
   *
   */
  void
  SortTargets()
  {
    std::sort(targets_.begin(), targets_.end());
  }

 private:
  /*############################################################################
   * Internal member variables
   *##########################################################################*/

  /// @brief Target positions of an MwCAS operation
  std::vector<size_t> targets_{};
};

#endif  // PMWCAS_BENCHMARK_ARRAY_OPERATION_HPP
