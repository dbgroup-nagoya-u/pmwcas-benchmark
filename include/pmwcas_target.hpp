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

#ifndef PMWCAS_BENCHMARK_ARRAY_PMWCAS_TARGET_HPP
#define PMWCAS_BENCHMARK_ARRAY_PMWCAS_TARGET_HPP

// C++ standard libraries
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

// external system libraries
#include <libpmemobj.h>

// local sources
#include "common.hpp"
#include "operation.hpp"

/**
 * @brief A class to deal with MwCAS target data and algorthms.
 *
 * @tparam Implementation A certain implementation of MwCAS algorithms.
 */
template <class Implementation>
class PMwCASTarget
{
 public:
  /*############################################################################
   * Public constructors and assignment operators
   *##########################################################################*/

  /**
   * @brief Construct a new PMwCASTarget object.
   *
   * @param pmem_dir_str A path to persistent memory for benchmarking.
   * @param array_cap The capacity of an array.
   * @param block_size The size of each memory block.
   */
  PMwCASTarget(  //
      const std::string &pmem_dir_str,
      const size_t array_cap,
      const size_t block_size);

  PMwCASTarget(const PMwCASTarget &) = delete;
  PMwCASTarget(PMwCASTarget &&) = delete;

  PMwCASTarget &operator=(const PMwCASTarget &obj) = delete;
  PMwCASTarget &operator=(PMwCASTarget &&) = delete;

  /*############################################################################
   * Public destructors
   *##########################################################################*/

  /**
   * @brief Destroy the PMwCASTarget object.
   *
   */
  ~PMwCASTarget();

  /*############################################################################
   * Setup/Teardown for workers
   *##########################################################################*/

  constexpr void
  SetUpForWorker()
  {
    // do nothing
  }

  constexpr void
  TearDownForWorker()
  {
    // do nothing
  }

  /*############################################################################
   * Public utilities
   *##########################################################################*/

  /**
   * @param pos The position in an array.
   * @return The current value.
   */
  auto GetValue(               //
      const size_t pos) const  //
      -> uint64_t;

  /**
   * @brief Perform a PMwCAS operation.
   *
   * @param ops An operation to be executed.
   * @return The number of executed operations (i.e., 1).
   */
  auto Execute(              //
      const Operation &ops)  //
      -> size_t;

 private:
  /*############################################################################
   * Internal utilities
   *##########################################################################*/

  /**
   * @param pos The position in memory blocks.
   * @return A target address.
   */
  auto GetAddr(          //
      const size_t pos)  //
      -> uint64_t *;

  /**
   * @brief Create an array on persistent memory.
   *
   * @param pmem_dir_str A path to persistent memory for benchmarking.
   * @param array_cap The capacity of an array.
   */
  void Initialize(  //
      const std::string &pmem_dir_str,
      const size_t array_cap);

  /*############################################################################
   * Internal member variables
   *##########################################################################*/

  /// @brief A path to persistent memory for benchmarking.
  std::string pmem_dir_str_{};

  /// @brief The pool for persistent memory.
  PMEMobjpool *pop_{nullptr};

  /// @brief The size of each block.
  size_t block_size_{256};

  /// @brief The size of the left-shift insruction instead of multiplication.
  size_t shift_num_{Log2(block_size_)};

  /// @brief An array on persistent memory.
  std::byte *root_addr_{nullptr};

  /// @brief A pool of PMwCAS descriptors.
  std::unique_ptr<Implementation> desc_pool_{nullptr};
};

#endif  // PMWCAS_BENCHMARK_ARRAY_PMWCAS_TARGET_HPP
