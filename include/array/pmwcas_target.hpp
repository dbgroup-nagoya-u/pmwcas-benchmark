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
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// external system libraries
#include <libpmem.h>
#include <libpmemobj.h>

// local sources
#include "array/operation.hpp"
#include "common.hpp"
#include "competitor.hpp"

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
   */
  PMwCASTarget(  //
      const std::string &pmem_dir_str,
      const size_t array_cap);

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
  ~PMwCASTarget()
  {
    pmwcas_desc_pool_ = nullptr;
    microsoft_pmwcas_desc_pool_ = nullptr;
    if (pop_ != nullptr) {
      pmemobj_close(pop_);
    }
    std::filesystem::remove_all(pmem_dir_str_);
  }

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
  auto
  GetValue(                    //
      const size_t pos) const  //
      -> uint64_t
  {
    const auto *addr = reinterpret_cast<const std::atomic_uint64_t *>(&arr_[pos]);
    return addr->load(kMORelax);
  }

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
   * Internal constants
   *##########################################################################*/

  /// @brief A layout name for the pool of PMwCAS descriptors.
  static constexpr char kTmpPath[] = "pmwcas_bench";

  /// @brief A layout name for the pool of PMwCAS descriptors.
  static constexpr char kPMwCASLayout[] = "pmwcas";

  /// @brief A layout name for the pool of PMwCAS descriptors.
  static constexpr char kMicrosoftPMwCASLayout[] = "microsoft_pmwcas";

  /// @brief A layout name for benchmarking with arrays.
  static constexpr char kArrayLayout[] = "array";

  /*############################################################################
   * Internal utilities
   *##########################################################################*/

  /**
   * @brief Create an array on persistent memory.
   *
   * @param pmem_dir_str A path to persistent memory for benchmarking.
   * @param array_cap The capacity of an array.
   */
  void
  Initialize(  //
      const std::string &pmem_dir_str,
      const size_t array_cap)
  {
    // reset a target directory
    pmem_dir_str_ = GetPath(pmem_dir_str, kTmpPath);
    std::filesystem::remove_all(pmem_dir_str_);
    std::filesystem::create_directories(pmem_dir_str_);

    // create a pool for persistent memory
    const size_t array_size = sizeof(uint64_t) * array_cap;
    const size_t pool_size = array_size + PMEMOBJ_MIN_POOL;
    const auto &path = GetPath(pmem_dir_str_, kArrayLayout);
    pop_ = pmemobj_create(path.c_str(), kArrayLayout, pool_size, kModeRW);
    if (pop_ == nullptr) throw std::runtime_error{pmemobj_errormsg()};

    const auto &root = pmemobj_root(pop_, array_size);
    arr_ = reinterpret_cast<uint64_t *>(pmemobj_direct(root));
  }

  /*############################################################################
   * Internal member variables
   *##########################################################################*/

  /// @brief A path to persistent memory for benchmarking.
  std::string pmem_dir_str_{};

  /// @brief The pool for persistent memory.
  PMEMobjpool *pop_{nullptr};

  /// @brief An array on persistent memory.
  uint64_t *arr_{nullptr};

  /// @brief A pool of our PMwCAS descriptors.
  std::unique_ptr<PMwCAS> pmwcas_desc_pool_{nullptr};

  /// @brief A pool of microsoft/pmwcas descriptors.
  std::unique_ptr<MicrosoftPMwCAS> microsoft_pmwcas_desc_pool_{nullptr};
};

#endif  // PMWCAS_BENCHMARK_ARRAY_PMWCAS_TARGET_HPP
