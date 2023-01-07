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

#ifndef PMWCAS_BENCHMARK_QUEUE_OPERATION_ENGINE_HPP
#define PMWCAS_BENCHMARK_QUEUE_OPERATION_ENGINE_HPP

// C++ standard libraries
#include <algorithm>
#include <optional>
#include <random>
#include <utility>
#include <vector>

class OperationEngine
{
 public:
  /*####################################################################################
   * Public constructors and assignment operators
   *##################################################################################*/

  /**
   * @brief Construct a new OperationEngine object.
   *
   */
  OperationEngine() = default;

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

  /**
   * @brief Generate a sequence of operations for benchmarks.
   *
   * @param n the number of operations to be executed by each worker.
   * @param random_seed a seed value for reproducibility.
   * @return the sequence of operations for benchmarks.
   */
  auto
  Generate(  //
      const size_t n,
      const size_t random_seed)  //
      -> std::vector<std::optional<uint64_t>>
  {
    constexpr size_t kMask = 1;
    std::mt19937_64 rand_engine{random_seed};

    // generate an operation-queue for benchmarking
    std::vector<std::optional<uint64_t>> operations{};
    operations.reserve(n);
    for (size_t i = 0; i < n; ++i) {
      const auto value = rand_engine();
      if ((value & kMask) == 0) {
        operations.emplace_back(value);
      } else {
        operations.emplace_back(std::nullopt);
      }
    }

    return operations;
  }
};

#endif  // PMWCAS_BENCHMARK_QUEUE_OPERATION_ENGINE_HPP
