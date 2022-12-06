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

// system headers
#include <libpmemobj++/mutex.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

// C++ standard libraries
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// external sources
#ifdef PMWCAS_BENCH_USE_MICROSOFT_PMWCAS
#include "pmwcas.h"
#endif

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

#ifndef PMWCAS_BENCH_USE_MICROSOFT_PMWCAS
      // not implemented yet
#else
      constexpr auto kPoolSize = PMEMOBJ_MIN_POOL * 1024;  // 8GB
      const uint32_t partition_num = worker_num;
      const uint32_t pool_capacity = partition_num * 1024;

      ::pmwcas::InitLibrary(
          pmwcas::PMDKAllocator::Create(pmwcas_path.c_str(), kPMwCASLayout.c_str(), kPoolSize),
          pmwcas::PMDKAllocator::Destroy,    //
          pmwcas::LinuxEnvironment::Create,  //
          pmwcas::LinuxEnvironment::Destroy);

      pmwcas_desc_pool_ = std::make_unique<PMwCAS>(pool_capacity, partition_num);
#endif
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
   * Internal member variables
   *##################################################################################*/

  PmemArray target_arr_{};

  /// a pmemobj_pool that contains a target array.
  ::pmem::obj::pool<Root> pool_{};

  /// a pointer to the root object in a pool.
  ::pmem::obj::persistent_ptr<Root> root_{};

#ifdef PMWCAS_BENCH_USE_MICROSOFT_PMWCAS
  /// the pool of PMwCAS descriptors.
  std::unique_ptr<PMwCAS> pmwcas_desc_pool_{nullptr};
#endif
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
#ifndef PMWCAS_BENCH_USE_MICROSOFT_PMWCAS
// not implemented yet
#else
  using PMwCASField = ::pmwcas::MwcTargetField<uint64_t>;

  while (true) {
    auto *desc = pmwcas_desc_pool_->AllocateDescriptor();
    auto *epoch = pmwcas_desc_pool_->GetEpoch();
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
#endif
}

#endif  // PMWCAS_BENCHMARK_ARRAY_PMWCAS_TARGET_HPP
