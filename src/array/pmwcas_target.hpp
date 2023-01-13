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
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

// local sources
#include "array/pmem_array.hpp"

/**
 * @brief A class to deal with MwCAS target data and algorthms.
 *
 * @tparam Implementation A certain implementation of MwCAS algorithms.
 */
template <class Implementation>
class PMwCASTarget
{
 public:
  /*####################################################################################
   * Type aliases
   *##################################################################################*/

  using Root = PmemArray::PmemRoot;

  /*####################################################################################
   * Public constructors/destructors
   *##################################################################################*/

  /**
   * @brief Construct a new PMwCASTarget object.
   *
   * @param pmem_dir_str the path to persistent memory for benchmarking.
   * @param worker_num the number of worker threads.
   */
  PMwCASTarget(  //
      const std::string &pmem_dir_str,
      const size_t worker_num)
      : target_arr_{pmem_dir_str}, pool_{target_arr_.GetPool()}, root_{pool_.root()}
  {
    // prepare descriptor pool for PMwCAS if needed
    if constexpr (std::is_same_v<Implementation, PMwCAS>) {
      const auto &pmwcas_path = GetPath(pmem_dir_str, kPMwCASLayout);
      pmwcas_desc_pool_ = std::make_unique<PMwCAS>(pmwcas_path, kPMwCASLayout);
    } else if constexpr (std::is_same_v<Implementation, MicrosoftPMwCAS>) {
      const auto &pmwcas_path = GetPath(pmem_dir_str, kMicrosoftPMwCASLayout);

      constexpr auto kPoolSize = PMEMOBJ_MIN_POOL * 1024;  // 8GB
      const uint32_t partition_num = worker_num;
      const uint32_t pool_capacity = partition_num * 1024;

      ::pmwcas::InitLibrary(
          pmwcas::PMDKAllocator::Create(pmwcas_path.c_str(), kMicrosoftPMwCASLayout, kPoolSize),
          pmwcas::PMDKAllocator::Destroy,    //
          pmwcas::LinuxEnvironment::Create,  //
          pmwcas::LinuxEnvironment::Destroy);

      microsoft_pmwcas_desc_pool_ = std::make_unique<MicrosoftPMwCAS>(pool_capacity, partition_num);
    }
  }

  /*####################################################################################
   * Public destructors
   *##################################################################################*/

  ~PMwCASTarget() = default;

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

  // define specializations below
  void Execute(const Operation &ops);

 private:
  /*####################################################################################
   * Internal constants
   *##################################################################################*/

  /// the layout name for the pool of PMwCAS descriptors.
  static constexpr char kPMwCASLayout[] = "pmwcas";

  /// the layout name for the pool of PMwCAS descriptors.
  static constexpr char kMicrosoftPMwCASLayout[] = "microsoft_pmwcas";

  /*####################################################################################
   * Internal member variables
   *##################################################################################*/

  PmemArray target_arr_{};

  /// a pmemobj_pool that contains a target array.
  ::pmem::obj::pool<Root> pool_{};

  /// a pointer to the root object in a pool.
  ::pmem::obj::persistent_ptr<Root> root_{};

  /// the pool of PMwCAS descriptors.
  std::unique_ptr<PMwCAS> pmwcas_desc_pool_{nullptr};

  /// the pool of microsoft/pmwcas descriptors.
  std::unique_ptr<MicrosoftPMwCAS> microsoft_pmwcas_desc_pool_{nullptr};
};

/*######################################################################################
 * Specializations for each PMwCAS implementations
 *####################################################################################*/

/**
 * @brief Specialization for the lock-based implementation.
 *
 * @tparam Lock
 * @param ops a target PMwCAS operation.
 */
template <>
inline void
PMwCASTarget<Lock>::Execute(const Operation &ops)
{
  try {
    ::pmem::obj::transaction::run(
        pool_,
        [&] {
          for (size_t i = 0; i < kTargetNum; ++i) {
            auto &&elem = root_->arr[ops.GetPosition(i)].get_rw();
            ++elem;
          }
        },
        root_->mtx);
  } catch (const ::pmem::transaction_error &e) {
    std::cerr << e.what() << std::endl;
    exit(EXIT_FAILURE);
  }
}

/**
 * @brief Specialization for the PMwCAS-based implementation.
 *
 * @tparam PMwCAS
 * @param ops a target PMwCAS operation.
 */
template <>
inline void
PMwCASTarget<PMwCAS>::Execute(const Operation &ops)
{
  using PMwCASDescriptor = ::dbgroup::atomic::pmwcas::PMwCASDescriptor;
  while (true) {
    auto *desc = pmwcas_desc_pool_->Get();
    for (size_t i = 0; i < kTargetNum; ++i) {
      auto *addr = &(root_->arr[ops.GetPosition(i)]);
      const auto old_val = PMwCASDescriptor::Read<uint64_t>(addr, std::memory_order_relaxed);
      const auto new_val = old_val + 1;
      desc->AddPMwCASTarget(addr, old_val, new_val, std::memory_order_relaxed);
    }
    if (desc->PMwCAS()) break;
  }
}

/**
 * @brief Specialization for the PMwCAS-based implementation.
 *
 * @tparam MicrosoftPMwCAS
 * @param ops a target PMwCAS operation.
 */
template <>
inline void
PMwCASTarget<MicrosoftPMwCAS>::Execute(const Operation &ops)
{
  using PMwCASField = ::pmwcas::MwcTargetField<uint64_t>;

  while (true) {
    auto *desc = microsoft_pmwcas_desc_pool_->AllocateDescriptor();
    auto *epoch = microsoft_pmwcas_desc_pool_->GetEpoch();
    epoch->Protect();
    for (size_t i = 0; i < kTargetNum; ++i) {
      auto *addr = reinterpret_cast<uint64_t *>(&(root_->arr[ops.GetPosition(i)]));
      const auto old_val = reinterpret_cast<PMwCASField *>(addr)->GetValueProtected();
      const auto new_val = old_val + 1;
      desc->AddEntry(addr, old_val, new_val);
    }
    auto success = desc->MwCAS();
    epoch->Unprotect();

    if (success) break;
  }
}

#endif  // PMWCAS_BENCHMARK_ARRAY_PMWCAS_TARGET_HPP
