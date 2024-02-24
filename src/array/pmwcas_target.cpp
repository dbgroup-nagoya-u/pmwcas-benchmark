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
#include "array/pmwcas_target.hpp"

// C++ standard libraries
#include <atomic>

/*##############################################################################
 * Local utilities
 *############################################################################*/

namespace
{

/// @brief A flag for indicating not-flushed values.
constexpr uint64_t kDirtyFlag = 1UL << 63UL;

/// @brief The size of words in bytes.
constexpr size_t kWordSize = 8;

/**
 * @brief Persist a given value if it includes a dirty flag.
 *
 * @param[in] word_addr An address of a target word.
 * @param[in,out] word A word that may be dirty.
 */
void
PersistDirtyValueIfNeeded(  //
    std::atomic_uint64_t *word_addr,
    uint64_t &word)
{
  auto dirty_v = word;
  while (word & kDirtyFlag) {
    pmem_persist(word_addr, kWordSize);
    word &= ~kDirtyFlag;
    if (word_addr->compare_exchange_strong(dirty_v, word, kMORelax)) break;
    word = dirty_v;
  }
}

/**
 * @param addr An address of a target word.
 * @return The current value of a given address.
 */
auto
PCASRead(        //
    void *addr)  //
    -> uint64_t
{
  auto *word_addr = reinterpret_cast<std::atomic_uint64_t *>(addr);
  auto word = word_addr->load(kMORelax);
  PersistDirtyValueIfNeeded(word_addr, word);
  return word;
}

/**
 * @param[in] addr An address of a target word.
 * @param[in,out] expected An expected value.
 * @param[in] desired A desired value.
 * @retval true if this PCAS operation succeeds.
 * @retval false otherwise.
 */
auto
PersistentCAS(  //
    void *addr,
    uint64_t &expected,
    const uint64_t desired)  //
    -> bool
{
  const auto orig_expected = expected;
  auto *word_addr = reinterpret_cast<std::atomic_uint64_t *>(addr);
  auto dirty_v = desired | kDirtyFlag;
  while (!word_addr->compare_exchange_strong(expected, dirty_v, kMORelax)) {
    PersistDirtyValueIfNeeded(word_addr, expected);
    if (expected != orig_expected) return false;
  }

  pmem_persist(addr, kWordSize);
  word_addr->compare_exchange_strong(dirty_v, desired, kMORelax);
  return true;
}

}  // namespace

/*##############################################################################
 * Public constructors and assignment operators
 *############################################################################*/

template <>
PMwCASTarget<PMwCAS>::PMwCASTarget(  //
    const std::string &pmem_dir_str,
    const size_t array_cap)
{
  Initialize(pmem_dir_str, array_cap);

  // prepare descriptor pool for PMwCAS
  const auto &pmwcas_path = GetPath(pmem_dir_str_, kPMwCASLayout);
  pmwcas_desc_pool_ = std::make_unique<PMwCAS>(pmwcas_path, kPMwCASLayout);
}

template <>
PMwCASTarget<MicrosoftPMwCAS>::PMwCASTarget(  //
    const std::string &pmem_dir_str,
    const size_t array_cap)
{
  Initialize(pmem_dir_str, array_cap);

  // prepare descriptor pool for PMwCAS
  const auto &pmwcas_path = GetPath(pmem_dir_str_, kMicrosoftPMwCASLayout);
  constexpr auto kPoolSize = PMEMOBJ_MIN_POOL * 1024;  // 8GB
  constexpr uint32_t kPartition = DBGROUP_MAX_THREAD_NUM;
  constexpr uint32_t kPoolCapacity = kPartition * 1024;

  ::pmwcas::InitLibrary(
      pmwcas::PMDKAllocator::Create(pmwcas_path.c_str(), kMicrosoftPMwCASLayout, kPoolSize),
      pmwcas::PMDKAllocator::Destroy,    //
      pmwcas::LinuxEnvironment::Create,  //
      pmwcas::LinuxEnvironment::Destroy);
  microsoft_pmwcas_desc_pool_ = std::make_unique<MicrosoftPMwCAS>(kPoolCapacity, kPartition);
}

template <>
PMwCASTarget<PCAS>::PMwCASTarget(  //
    const std::string &pmem_dir_str,
    const size_t array_cap)
{
  Initialize(pmem_dir_str, array_cap);
}

/*##############################################################################
 * Executed procedures for each implementation
 *############################################################################*/

template <>
auto
PMwCASTarget<PMwCAS>::Execute(  //
    const Operation &ops)       //
    -> size_t
{
  const auto &positions = ops.GetPositions();
  while (true) {
    auto *desc = pmwcas_desc_pool_->Get();
    for (const auto pos : positions) {
      auto *addr = &(arr_[pos]);
      const auto old_val = ::dbgroup::atomic::pmwcas::Read<uint64_t>(addr, kMORelax);
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
  auto *epoch = microsoft_pmwcas_desc_pool_->GetEpoch();
  epoch->Protect();
  while (true) {
    auto *desc = microsoft_pmwcas_desc_pool_->AllocateDescriptor();
    for (const auto pos : positions) {
      auto *addr = reinterpret_cast<uint64_t *>(&(arr_[pos]));
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

  auto *addr = &(arr_[positions.at(0)]);
  while (true) {
    auto old_val = PCASRead(addr);
    for (size_t i = 0; true; ++i) {
      if (PersistentCAS(addr, old_val, old_val + 1)) goto pcas_out;
      if (i >= ::dbgroup::atomic::pmwcas::kRetryNum) break;
      PMWCAS_SPINLOCK_HINT
    }

    std::this_thread::sleep_for(::dbgroup::atomic::pmwcas::kShortSleep);
  }

pcas_out:
  return 1;
}
