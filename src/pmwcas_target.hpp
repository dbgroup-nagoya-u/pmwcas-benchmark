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

#ifndef MWCAS_BENCHMARK_MWCAS_TARGET_H
#define MWCAS_BENCHMARK_MWCAS_TARGET_H

#include <memory>
#include <thread>
#include <vector>

#include "common.hpp"
#include "operation.hpp"
#include "pmwcas.h"

// declare PMwCAS's descriptor pool globally in order to define a templated worker class
inline std::unique_ptr<PMwCAS> pmwcas_desc_pool = nullptr;

/**
 * @brief A class to deal with MwCAS target data and algorthms.
 *
 * @tparam Implementation A certain implementation of MwCAS algorithms.
 */
template <class Implementation>
class MwCASTarget
{
 public:
  /*################################################################################################
   * Public constructors/destructors
   *##############################################################################################*/

  MwCASTarget(  //
      const size_t total_field_num,
      const size_t init_thread_num,
      const size_t worker_num)
      : target_fields_{total_field_num, nullptr}
  {
    // a lambda function to initialize target fields
    auto f = [&](const size_t thread_id, const size_t n) {
      for (size_t i = 0, id = thread_id; i < n; ++i, id += init_thread_num) {
        target_fields_[id] = new uint64_t{0};
      }
    };

    // prepare MwCAS target fields with multi-threads
    std::vector<std::thread> threads;
    for (size_t i = 0; i < init_thread_num; ++i) {
      const size_t n = (total_field_num + ((init_thread_num - 1) - i)) / init_thread_num;
      threads.emplace_back(f, i, n);
    }
    for (auto &&t : threads) t.join();

    // prepare descriptor pool for PMwCAS if needed
    if constexpr (std::is_same_v<Implementation, PMwCAS>) {
      // prepare PMwCAS descriptor pool
      ::pmwcas::InitLibrary(pmwcas::DefaultAllocator::Create, pmwcas::DefaultAllocator::Destroy,
                            pmwcas::LinuxEnvironment::Create, pmwcas::LinuxEnvironment::Destroy);
      pmwcas_desc_pool = std::make_unique<PMwCAS>(static_cast<uint32_t>(8192 * worker_num),
                                                  static_cast<uint32_t>(worker_num));
    }
  }

  /*################################################################################################
   * Public destructors
   *##############################################################################################*/

  ~MwCASTarget()
  {
    for (auto &&addr : target_fields_) {
      delete addr;
    }
  }

  /*################################################################################################
   * Public utility functions
   *##############################################################################################*/

  void Execute(const Operation &ops);

  const std::vector<uint64_t *> &
  ReferTargetFields() const
  {
    return target_fields_;
  }

 private:
  /*################################################################################################
   * Internal member variables
   *##############################################################################################*/

  /// target fields of MwCAS operations
  std::vector<uint64_t *> target_fields_;
};

/*##################################################################################################
 * Specializations for each MwCAS implementations
 *################################################################################################*/

template <>
inline void
MwCASTarget<MwCAS>::Execute(const Operation &ops)
{
  while (true) {
    MwCAS desc{};
    for (size_t i = 0; i < kTargetNum; ++i) {
      const auto addr = ops.GetAddr(i);
      const auto old_val = MwCAS::Read<size_t>(addr);
      const auto new_val = old_val + 1;
      desc.AddMwCASTarget(addr, old_val, new_val);
    }

    if (desc.MwCAS()) break;
  }
}

template <>
inline void
MwCASTarget<PMwCAS>::Execute(const Operation &ops)
{
  using PMwCASField = ::pmwcas::MwcTargetField<uint64_t>;

  while (true) {
    auto desc = pmwcas_desc_pool->AllocateDescriptor();
    auto epoch = pmwcas_desc_pool->GetEpoch();
    epoch->Protect();
    for (size_t i = 0; i < kTargetNum; ++i) {
      const auto addr = ops.GetAddr(i);
      const auto old_val = reinterpret_cast<PMwCASField *>(addr)->GetValueProtected();
      const auto new_val = old_val + 1;
      desc->AddEntry(addr, old_val, new_val);
    }
    auto success = desc->MwCAS();
    epoch->Unprotect();

    if (success) break;
  }
}

template <>
inline void
MwCASTarget<AOPT>::Execute(const Operation &ops)
{
  while (true) {
    auto desc = AOPT::GetDescriptor();
    for (size_t i = 0; i < kTargetNum; ++i) {
      const auto addr = ops.GetAddr(i);
      const auto old_val = AOPT::Read<size_t>(addr);
      const auto new_val = old_val + 1;
      desc->AddMwCASTarget(addr, old_val, new_val);
    }

    if (desc->MwCAS()) break;
  }
}

template <>
inline void
MwCASTarget<SingleCAS>::Execute(const Operation &ops)
{
  for (size_t i = 0; i < kTargetNum; ++i) {
    auto target = reinterpret_cast<SingleCAS *>(ops.GetAddr(i));
    auto old_val = target->load(std::memory_order_relaxed);
    auto new_val = old_val + 1;
    while (!target->compare_exchange_weak(old_val, new_val, std::memory_order_relaxed)) {
      new_val = old_val + 1;
    }
  }
}

#endif  // MWCAS_BENCHMARK_MWCAS_TARGET_H
