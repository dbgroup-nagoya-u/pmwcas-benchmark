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

#ifndef PMWCAS_BENCHMARK_ARRAY_OPERATION_ENGINE_HPP
#define PMWCAS_BENCHMARK_ARRAY_OPERATION_ENGINE_HPP

// C++ standard libraries
#include <algorithm>
#include <random>
#include <utility>
#include <vector>

// external sources
#include "random/zipf.hpp"

// local sources
#include "array/operation.hpp"

class OperationEngine
{
  /*####################################################################################
   * Type aliases
   *##################################################################################*/

  using ZipfDist_t = ::dbgroup::random::ApproxZipfDistribution<size_t>;

 public:
  /*####################################################################################
   * Public constructors and assignment operators
   *##################################################################################*/

  OperationEngine(  //
      const double skew_param,
      const size_t random_seed)
      : zipf_dist_{0, kElementNum - 1, skew_param}
  {
    pos_index_.reserve(kElementNum);
    for (size_t i = 0; i < kElementNum; ++i) {
      pos_index_.emplace_back(i);
    }
    std::mt19937_64 rand_engine{random_seed};
    std::shuffle(pos_index_.begin(), pos_index_.end(), rand_engine);
  }

  OperationEngine(const OperationEngine &) = default;
  OperationEngine &operator=(const OperationEngine &obj) = default;
  OperationEngine(OperationEngine &&) = default;
  OperationEngine &operator=(OperationEngine &&) = default;

  /*####################################################################################
   * Public destructors
   *##################################################################################*/

  ~OperationEngine() = default;

  /*####################################################################################
   * Public utility functions
   *##################################################################################*/

  auto
  Generate(  //
      const size_t n,
      const size_t random_seed)  //
      -> std::vector<Operation>
  {
    std::mt19937_64 rand_engine{random_seed};

    // generate an operation-queue for benchmarking
    std::vector<Operation> operations;
    operations.reserve(n);
    for (size_t i = 0; i < n; ++i) {
      // select target addresses for i-th operation
      Operation ops{};
      for (size_t j = 0; j < kTargetNum; ++j) {
        auto pos = pos_index_.at(zipf_dist_(rand_engine));
        while (!ops.SetAddrIfUnique(j, pos)) {
          // continue until the different target is selected
          pos = pos_index_.at(zipf_dist_(rand_engine));
        }
      }
      ops.SortTargets();

      operations.emplace_back(std::move(ops));
    }

    return operations;
  }

 private:
  /*####################################################################################
   * Internal member variables
   *##################################################################################*/

  /// the index for indicating actual positions in an array.
  std::vector<size_t> pos_index_{};

  /// a random value generator according to Zipf's law
  ZipfDist_t zipf_dist_{};
};

#endif  // PMWCAS_BENCHMARK_ARRAY_OPERATION_ENGINE_HPP
