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

#ifndef MWCAS_BENCHMARK_OPERATION_ENGINE_H
#define MWCAS_BENCHMARK_OPERATION_ENGINE_H

#include <random>
#include <utility>
#include <vector>

#include "common.hpp"
#include "operation.hpp"
#include "random/zipf.hpp"

class OperationEngine
{
  /*################################################################################################
   * Type aliases
   *##############################################################################################*/

  using ZipfGenerator = ::dbgroup::random::zipf::ZipfGenerator;

 public:
  /*################################################################################################
   * Public constructors and assignment operators
   *##############################################################################################*/

  OperationEngine(  //
      const std::vector<uint64_t *> &target_fields,
      const double skew_parameter)
      : target_fields_{target_fields}, zipf_engine_{target_fields_.size(), skew_parameter}
  {
  }

  OperationEngine(const OperationEngine &) = default;
  OperationEngine &operator=(const OperationEngine &obj) = default;
  OperationEngine(OperationEngine &&) = default;
  OperationEngine &operator=(OperationEngine &&) = default;

  /*################################################################################################
   * Public destructors
   *##############################################################################################*/

  ~OperationEngine() = default;

  /*################################################################################################
   * Public utility functions
   *##############################################################################################*/

  std::vector<Operation>
  Generate(  //
      const size_t n,
      const size_t random_seed)
  {
    std::mt19937_64 rand_engine{random_seed};

    // generate an operation-queue for benchmarking
    std::vector<Operation> operations;
    operations.reserve(n);
    for (size_t i = 0; i < n; ++i) {
      // select target addresses for i-th operation
      Operation ops{};
      for (size_t j = 0; j < kTargetNum; ++j) {
        auto addr = target_fields_[zipf_engine_(rand_engine)];
        while (!ops.SetAddr(j, addr)) addr = target_fields_[zipf_engine_(rand_engine)];
      }
      ops.SortTargets();

      operations.emplace_back(std::move(ops));
    }

    return operations;
  }

 private:
  /*################################################################################################
   * Internal member variables
   *##############################################################################################*/

  /// a reference to MwCAS target fields
  const std::vector<uint64_t *> &target_fields_;

  /// a random engine according to Zipf's law
  ZipfGenerator zipf_engine_;
};

#endif  // MWCAS_BENCHMARK_OPERATION_ENGINE_H
