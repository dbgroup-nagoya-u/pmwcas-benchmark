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

#ifndef PMWCAS_BENCHMARK_QUEUE_BENCH_TARGET_HPP
#define PMWCAS_BENCHMARK_QUEUE_BENCH_TARGET_HPP

// C++ standard libraries
#include <optional>
#include <string>

/**
 * @brief A class to deal with MwCAS target data and algorthms.
 *
 * @tparam Queue A certain implementation of persistent queues.
 */
template <class Queue>
class BenchTarget
{
 public:
  /*####################################################################################
   * Public constructors/destructors
   *##################################################################################*/

  /**
   * @brief Construct a new BenchTarget object.
   *
   * @param pmem_dir_str the path to persistent memory for benchmarking.
   */
  explicit BenchTarget(const std::string &pmem_dir_str) : queue_{pmem_dir_str} {}

  /*####################################################################################
   * Public destructors
   *##################################################################################*/

  ~BenchTarget() = default;

  /*####################################################################################
   * Setup/Teardown for workers
   *##################################################################################*/

  void
  SetUpForWorker()
  {
    // do nothing
  }

  void
  TearDownForWorker()
  {
    // do nothing
  }

  /*####################################################################################
   * Executed procedures for each implementation
   *##################################################################################*/

  void
  Execute(const std::optional<uint64_t> &ops)
  {
    if (ops) {
      queue_.Push(*ops);
    } else {
      queue_.Pop();
    }
  }

 private:
  /*####################################################################################
   * Internal member variables
   *##################################################################################*/

  Queue queue_{};
};

#endif  // PMWCAS_BENCHMARK_QUEUE_BENCH_TARGET_HPP
