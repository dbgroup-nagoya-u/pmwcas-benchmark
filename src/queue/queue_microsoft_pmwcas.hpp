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

#ifndef PMWCAS_BENCHMARK_QUEUE_QUEUE_MICROSOFT_PMWCAS_HPP
#define PMWCAS_BENCHMARK_QUEUE_QUEUE_MICROSOFT_PMWCAS_HPP

// C++ standard libraries
#include <exception>
#include <iostream>
#include <optional>
#include <sstream>
#include <utility>

// external libraries
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/transaction.hpp>
#include <libpmemobj++/utils.hpp>

// external sources
#include "memory/epoch_based_gc.hpp"

#ifdef PMWCAS_BENCH_USE_MICROSOFT_PMWCAS
#include "pmwcas.h"
#endif

// local sources
#include "common.hpp"
#include "element_holder.hpp"
#include "queue/node_pmwcas.hpp"

/**
 * @brief A class for representing a queue on persistent memory.
 *
 * The internal data structure of this class is a linked list, and this uses
 * `microsoft/pmwcas` for concurrency controls.
 */
template <class T>
class QueueWithMicrosoftPMwCAS
{
 public:
  /*####################################################################################
   * Type aliases
   *##################################################################################*/

  using PMwCASField = ::pmwcas::MwcTargetField<size_t>;
  using Node_t = Node<T>;
  using ElementHolder_t = ElementHolder<::pmem::obj::persistent_ptr<Node_t> *>;
  using GC_t = ::dbgroup::memory::EpochBasedGC<NodeTarget>;
  using GarbageNode_t = ::dbgroup::memory::GarbageNodeOnPMEM<NodeTarget>;

  /*####################################################################################
   * Public constructors/destructors
   *##################################################################################*/

  /**
   * @brief Construct a new QueueWithMicrosoftPMwCAS object.
   *
   * @param path the path of persistent memory for benchmarking.
   */
  QueueWithMicrosoftPMwCAS(  //
      const std::string &pmem_dir_str,
      const size_t worker_num)
  {
    // prepare a pool instance
    const auto &pool_path = GetPath(pmem_dir_str, kQueueLayout);
    try {
      if (std::filesystem::exists(pool_path)) {
        pool_ = ::pmem::obj::pool<Root>::open(pool_path, kQueueLayout);
        root_ = pool_.root();
        pool_uuid_ = root_.raw().pool_uuid_lo;
      } else {
        constexpr size_t kSize = PMEMOBJ_MIN_POOL * 256 * 2;  // 2GB
        pool_ = ::pmem::obj::pool<Root>::create(pool_path, kQueueLayout, kSize, CREATE_MODE_RW);
        root_ = pool_.root();
        pool_uuid_ = root_.raw().pool_uuid_lo;

        // create a dummy node
        ::pmem::obj::flat_transaction::run(pool_, [&] {  //
          pmemobj_tx_add_range(root_.raw(), 0, 2 * sizeof(PMEMoid));
          root_->head = ::pmem::obj::make_persistent<Node_t>(0, pool_uuid_).raw();
          root_->tail = root_->head;
        });
      }
    } catch (const std::exception &e) {
      std::cerr << e.what() << std::endl;
      exit(EXIT_FAILURE);
    }

    // prepare a descriptor pool for PMwCAS
    const auto &pmwcas_path = GetPath(pmem_dir_str, kPMwCASLayout);
    constexpr auto kPoolSize = PMEMOBJ_MIN_POOL * 1024;  // 8GB
    const uint32_t partition_num = worker_num;
    const uint32_t pool_capacity = partition_num * 1024;
    ::pmwcas::InitLibrary(
        pmwcas::PMDKAllocator::Create(pmwcas_path.c_str(), kPMwCASLayout, kPoolSize),
        pmwcas::PMDKAllocator::Destroy,    //
        pmwcas::LinuxEnvironment::Create,  //
        pmwcas::LinuxEnvironment::Destroy);
    pmwcas_desc_pool_ = std::make_unique<PMwCAS>(pool_capacity, partition_num);

    // prepare a garbage collector
    gc_.SetHeadAddrOnPMEM<NodeTarget>(&(pool_.root()->gc_head));
    gc_.StartGC();
  }

  /*####################################################################################
   * Public destructors
   *##################################################################################*/

  ~QueueWithMicrosoftPMwCAS()
  {
    gc_.StopGC();
    pool_.close();
  }

  /*####################################################################################
   * Public utilities
   *##################################################################################*/

  /**
   * @brief Inserts a new element at the end of the queue.
   *
   */
  void
  Push(const T &value)
  {
    // create a new node
    auto *tmp_node_addr = ReserveNodeAddress();
    try {
      ::pmem::obj::flat_transaction::run(pool_, [&] {  //
        *tmp_node_addr = ::pmem::obj::make_persistent<Node_t>(value, pool_uuid_);
      });
    } catch (const std::exception &e) {
      std::cerr << e.what() << std::endl;
    }

    // insert the new node by using PMwCAS
    auto *tmp_addr = &(tmp_node_addr->raw_ptr()->off);
    auto *tail_addr = &(root_->tail.off);
    const auto new_node = *tmp_addr;
    while (true) {
      [[maybe_unused]] const ::pmwcas::EpochGuard desc_guard{pmwcas_desc_pool_->GetEpoch()};
      auto *desc = pmwcas_desc_pool_->AllocateDescriptor();

      // prepare PMwCAS targets
      const auto old_tail = ReadNodeProtected(tail_addr);
      ::pmem::obj::persistent_ptr<Node_t> tail_node{PMEMoid{pool_uuid_, old_tail}};
      desc->AddEntry(&(tail_node->next.off), kNullPtr, new_node);
      desc->AddEntry(tail_addr, old_tail, new_node);
      desc->AddEntry(tmp_addr, new_node, kNullPtr);  // to prevent memory leak

      if (desc->MwCAS()) break;
    }
  }

  /**
   * @brief Removes the first element in the queue.
   *
   */
  auto
  Pop()  //
      -> std::optional<T>
  {
    [[maybe_unused]] const auto &gc_guard = gc_.CreateEpochGuard();

    // prepare a temporary region for a node to be removed
    auto *tmp_node_addr = ReserveNodeAddress();
    *tmp_node_addr = PMEMoid{pool_uuid_, kNullPtr};
    pool_.persist(tmp_node_addr, sizeof(PMEMoid));

    // remove the head node by using PMwCAS
    auto *tmp_addr = &(tmp_node_addr->raw_ptr()->off);
    auto *head_addr = &(root_->head.off);
    while (true) {
      [[maybe_unused]] const ::pmwcas::EpochGuard desc_guard{pmwcas_desc_pool_->GetEpoch()};

      // check this queue has any element
      const auto old_head = ReadNodeProtected(head_addr);
      ::pmem::obj::persistent_ptr<Node_t> head_node{PMEMoid{pool_uuid_, old_head}};
      const auto new_head = ReadNodeProtected(&(head_node->next.off));
      if (new_head == kNullPtr) return std::nullopt;

      // prepare PMwCAS targets
      auto *desc = pmwcas_desc_pool_->AllocateDescriptor();
      desc->AddEntry(&(root_->head.off), old_head, new_head);
      desc->AddEntry(tmp_addr, kNullPtr, old_head);  // to prevent memory leak

      if (desc->MwCAS()) {
        // the head node has been popped, so remove it and return its content
        gc_.AddGarbage<NodeTarget>(tmp_node_addr->raw_ptr(), pool_);
        head_node = ::pmem::obj::persistent_ptr<Node_t>{PMEMoid{pool_uuid_, new_head}};
        return head_node->value;
      }
    }
  }

 private:
  /*####################################################################################
   * Internal constants
   *##################################################################################*/

  /// @brief The layout name for this class.
  static constexpr char kQueueLayout[] = "queue_microsoft_pmwcas";

  /// @brief The layout name for this class.
  static constexpr char kPMwCASLayout[] = "microsoft_pmwcas_for_queue";

  /*####################################################################################
   * Internal classes
   *##################################################################################*/

  struct Root {
    /// @brief The head of the queue.
    PMEMoid head{OID_NULL};

    /// @brief The tail of the queue.
    PMEMoid tail{OID_NULL};

    /// @brief The head of a linked list for GC.
    ::pmem::obj::persistent_ptr<GarbageNode_t> gc_head{nullptr};

    /// @brief A temporal region to be stored node pointers.
    ::pmem::obj::persistent_ptr<Node_t> tmp_nodes[kMaxThreadNum]{};
  };

  /*####################################################################################
   * Internal utility functions
   *##################################################################################*/

  static auto
  ReadNodeProtected(size_t *addr)  //
      -> size_t
  {
    return reinterpret_cast<PMwCASField *>(addr)->GetValueProtected();
  }

  auto
  ReserveNodeAddress()  //
      -> ::pmem::obj::persistent_ptr<Node_t> *
  {
    thread_local std::unique_ptr<ElementHolder_t> ptr = nullptr;

    while (!ptr) {
      for (size_t i = 0; i < kMaxThreadNum; ++i) {
        auto is_reserved = reserve_arr_[i].load(std::memory_order_relaxed);
        if (!is_reserved) {
          reserve_arr_[i].compare_exchange_strong(is_reserved, true, std::memory_order_relaxed);
          if (!is_reserved) {
            auto &&root = pool_.root();
            ptr = std::make_unique<ElementHolder_t>(i, reserve_arr_, &(root->tmp_nodes[i]));
            break;
          }
        }
      }
    }

    return ptr->Get();
  }

  /*####################################################################################
   * Internal member variables
   *##################################################################################*/

  ::pmem::obj::pool<Root> pool_{};

  ::pmem::obj::persistent_ptr<Root> root_{nullptr};

  size_t pool_uuid_{kNullPtr};

  /// @brief A garbage collector for nodes.
  GC_t gc_{kGCInterval, kThreadNum};

  /// @brief The pool of PMwCAS descriptors.
  std::unique_ptr<PMwCAS> pmwcas_desc_pool_{nullptr};

  /// @brief An array representing the occupied state of the descriptor pool
  std::shared_ptr<std::atomic_bool[]> reserve_arr_{new std::atomic_bool[kMaxThreadNum]};
};

#endif  // PMWCAS_BENCHMARK_QUEUE_QUEUE_MICROSOFT_PMWCAS_HPP
