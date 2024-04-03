/*
 * Copyright 2024 Database Group, Nagoya University
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

// corresponding header
#include "pmwcas_target.hpp"

// C++ standard libraries
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>

// system headers
#include <sys/stat.h>

// external system libraries
#include <libpmemobj.h>

// external libraries
#include "pmem/atomic/atomic.hpp"
#include "pmwcas.h"

// local sources
#include "common.hpp"
#include "competitor.hpp"
#include "operation.hpp"

namespace
{
/*##############################################################################
 * Local constants
 *############################################################################*/

/// @brief A layout name for the pool of PMwCAS descriptors.
constexpr char kBenchPath[] = "pmwcas_bench";

/// @brief A layout name for the pool of PMwCAS descriptors.
constexpr char kPMwCASName[] = "pmwcas";

/// @brief A layout name for the pool of microsoft/pmwcas descriptors.
constexpr char kMicrosoftPMwCASName[] = "microsoft_pmwcas";

/// @brief A layout name for benchmarking with arrays.
constexpr char kArrayName[] = "array";

/// @brief File permission for pmemobj_pool.
constexpr auto kModeRW = S_IRUSR | S_IWUSR;  // NOLINT

/// @brief An alias of std::memory_order_relaxed.
constexpr std::memory_order kMORelax = std::memory_order_relaxed;

}  // namespace

/*##############################################################################
 * Public constructors and destructors
 *############################################################################*/

template <>
PMwCASTarget<PMwCAS>::PMwCASTarget(  //
    const std::string &pmem_dir_str,
    const size_t array_cap,
    const size_t block_size)
    : block_size_{block_size}
{
  Initialize(pmem_dir_str, array_cap);

  // prepare descriptor pool for PMwCAS
  const auto &pmwcas_path = GetPath(pmem_dir_str_, kPMwCASName);
  desc_pool_ = std::make_unique<PMwCAS>(pmwcas_path, kPMwCASName);
}

template <>
PMwCASTarget<MicrosoftPMwCAS>::PMwCASTarget(  //
    const std::string &pmem_dir_str,
    const size_t array_cap,
    const size_t block_size)
    : block_size_{block_size}
{
  Initialize(pmem_dir_str, array_cap);

  // prepare descriptor pool for PMwCAS
  const auto &pmwcas_path = GetPath(pmem_dir_str_, kMicrosoftPMwCASName);
  constexpr auto kPoolSize = PMEMOBJ_MIN_POOL * 1024;  // 8GB
  constexpr uint32_t kPartition = DBGROUP_MAX_THREAD_NUM;
  constexpr uint32_t kPoolCapacity = kPartition * 1024;

  ::pmwcas::InitLibrary(
      pmwcas::PMDKAllocator::Create(pmwcas_path.c_str(), kMicrosoftPMwCASName, kPoolSize),
      pmwcas::PMDKAllocator::Destroy,    //
      pmwcas::LinuxEnvironment::Create,  //
      pmwcas::LinuxEnvironment::Destroy);
  desc_pool_ = std::make_unique<MicrosoftPMwCAS>(kPoolCapacity, kPartition);
}

template <>
PMwCASTarget<PCAS>::PMwCASTarget(  //
    const std::string &pmem_dir_str,
    const size_t array_cap,
    const size_t block_size)
    : block_size_{block_size}
{
  Initialize(pmem_dir_str, array_cap);
}

template <class Implementation>
PMwCASTarget<Implementation>::~PMwCASTarget()
{
  desc_pool_ = nullptr;
  if (pop_ != nullptr) {
    pmemobj_close(pop_);
  }
  std::filesystem::remove_all(pmem_dir_str_);
}

/*##############################################################################
 * Public APIs
 *############################################################################*/

template <class Implementation>
auto
PMwCASTarget<Implementation>::GetValue(  //
    const size_t pos) const              //
    -> uint64_t
{
  const void *addr = root_addr_ + (pos << shift_num_);
  return reinterpret_cast<const std::atomic_uint64_t *>(addr)->load(kMORelax);
}

template <>
auto
PMwCASTarget<PMwCAS>::Execute(  //
    const Operation &ops)       //
    -> size_t
{
  const auto &positions = ops.GetPositions();
  while (true) {
    auto *desc = desc_pool_->Get();
    for (const auto pos : positions) {
      auto *addr = GetAddr(pos);
      const auto old_val = ::dbgroup::pmem::atomic::PLoad(addr, kMORelax);
      desc->Add(addr, old_val, old_val + 1, kMORelax);
    }
    if (desc->PMwCAS()) break;
  }

  return 1;
}

template <>
auto
PMwCASTarget<MicrosoftPMwCAS>::Execute(  //
    const Operation &ops)                //
    -> size_t
{
  using PMwCASField = ::pmwcas::MwcTargetField<uint64_t>;

  const auto &positions = ops.GetPositions();
  auto *epoch = desc_pool_->GetEpoch();
  epoch->Protect();
  while (true) {
    auto *desc = desc_pool_->AllocateDescriptor();
    for (const auto pos : positions) {
      auto *addr = GetAddr(pos);
      const auto old_val = reinterpret_cast<PMwCASField *>(addr)->GetValueProtected();
      desc->AddEntry(addr, old_val, old_val + 1);
    }
    if (desc->MwCAS()) break;
  }
  epoch->Unprotect();

  return 1;
}

template <>
auto
PMwCASTarget<PCAS>::Execute(  //
    const Operation &ops)     //
    -> size_t
{
  const auto &positions = ops.GetPositions();
  assert(positions.size() == 1);

  auto *addr = GetAddr(positions.front());
  auto old_val = ::dbgroup::pmem::atomic::PLoad(addr, kMORelax);
  while (!::dbgroup::pmem::atomic::PCAS(addr, old_val, old_val + 1, kMORelax, kMORelax)) {
    // continue until PCAS succeeds
  }

  return 1;
}

/*##############################################################################
 * Internal APIs
 *############################################################################*/

template <class Implementation>
auto
PMwCASTarget<Implementation>::GetAddr(  //
    const size_t pos)                   //
    -> uint64_t *
{
  return reinterpret_cast<uint64_t *>(root_addr_ + (pos << shift_num_));
}

template <class Implementation>
void
PMwCASTarget<Implementation>::Initialize(  //
    const std::string &pmem_dir_str,
    const size_t array_cap)
{
  // reset a target directory
  pmem_dir_str_ = GetPath(pmem_dir_str, kBenchPath);
  std::filesystem::remove_all(pmem_dir_str_);
  std::filesystem::create_directories(pmem_dir_str_);

  // create a pool for persistent memory
  const size_t array_size = block_size_ * (array_cap + 1);
  const size_t pool_size = array_size + PMEMOBJ_MIN_POOL;
  const auto &path = GetPath(pmem_dir_str_, kArrayName);
  pop_ = pmemobj_create(path.c_str(), kArrayName, pool_size, kModeRW);
  if (pop_ == nullptr) throw std::runtime_error{pmemobj_errormsg()};

  const auto bit_mask = block_size_ - 1;
  auto &&root = pmemobj_root(pop_, array_size);
  root.off = (root.off + bit_mask) & ~bit_mask;
  root_addr_ = reinterpret_cast<std::byte *>(pmemobj_direct(root));
}

/*##############################################################################
 * Explicit instantiation definitions
 *############################################################################*/

template class PMwCASTarget<PMwCAS>;
template class PMwCASTarget<MicrosoftPMwCAS>;
template class PMwCASTarget<PCAS>;
