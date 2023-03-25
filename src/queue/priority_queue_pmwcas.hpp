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

#ifndef PMWCAS_BENCHMARK_QUEUE_PRIORITY_QUEUE_PMWCAS_HPP
#define PMWCAS_BENCHMARK_QUEUE_PRIORITY_QUEUE_PMWCAS_HPP

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
#include "pmwcas/descriptor_pool.hpp"

// local sources
#include "common.hpp"
#include "element_holder.hpp"
#include "queue_node_pmwcas.hpp"

/**
 * @brief A class for representing a queue on persistent memory.
 *
 * The internal data structure of this class is a linked list, and this uses
 * `microsoft/pmwcas` for concurrency controls.
 */
template <class T>
class PriorityQueueWithPMwCAS
{
 public:
  /*####################################################################################
   * Type aliases
   *##################################################################################*/

  using Node_t = Node<T>;
  using ElementHolder_t = ElementHolder<::pmem::obj::persistent_ptr<Node_t> *>;
  using NodeTarget_t = NodeTarget<!kReusePageOnPMEM>;
  using GC_t = ::dbgroup::memory::EpochBasedGC<NodeTarget_t>;
  using GarbageNode_t = ::dbgroup::memory::GarbageNodeOnPMEM<NodeTarget_t>;
  using DescriptorPool = ::dbgroup::atomic::pmwcas::DescriptorPool;
  using PMwCASDescriptor = ::dbgroup::atomic::pmwcas::PMwCASDescriptor;

  /*####################################################################################
   * Public constructors/destructors
   *##################################################################################*/

  /**
   * @brief Construct a new PriorityQueueWithMicrosoftPMwCAS object.
   *
   * @param pmem_dir_str the path of persistent memory for benchmarking.
   */
  PriorityQueueWithPMwCAS(const std::string &pmem_dir_str)
  {
    // prepare a pool instance
    const auto &pool_path = GetPath(pmem_dir_str, kQueueLayout);
    if (std::filesystem::exists(pool_path)) {
      RecoverQueuePool(pool_path);
    } else {
      CreateQueuePool(pool_path);
    }

    // prepare a garbage collector
    gc_ = std::make_unique<GC_t>(kGCInterval, kGCThreadNum);
    gc_->SetHeadAddrOnPMEM<NodeTarget_t>(&(pool_.root()->gc_head));
    gc_->StartGC();

    // prepare a descriptor pool for PMwCAS
    const auto &pmwcas_path = GetPath(pmem_dir_str, kPMwCASLayout);
    pmwcas_desc_pool_ = std::make_unique<DescriptorPool>(pmwcas_path, kPMwCASLayout);
  }

  /*####################################################################################
   * Public destructors
   *##################################################################################*/

  ~PriorityQueueWithPMwCAS()
  {
    gc_ = nullptr;
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
    [[maybe_unused]] const auto &gc_guard = gc_->CreateEpochGuard();

    // create a new node
    auto *tmp_node_addr = ReserveNodeAddress();
    try {
      ::pmem::obj::flat_transaction::run(pool_, [&] {  //
        *tmp_node_addr = ::pmem::obj::make_persistent<Node_t>(value, pool_uuid_);
      });
    } catch (const std::exception &e) {
      std::cerr << e.what() << std::endl;
    }

    // search the inserting position
    auto *tmp_addr = &(tmp_node_addr->raw_ptr()->off);
    const auto new_ptr = *tmp_addr;
    auto *next_addr = &(root_->head.off);
    while (true) {
      // check the next node has smaller value
      auto next_ptr = PMwCASDescriptor::Read<uint64_t>(next_addr, std::memory_order_relaxed);
      ::pmem::obj::persistent_ptr<Node_t> next_node{PMEMoid{pool_uuid_, next_ptr}};
      if (next_node != nullptr && next_node->value > value) {
        // go to the next node
        next_addr = &(next_node->next.off);
        continue;
      }

      // set the next pointer in the new node
      (*tmp_node_addr)->next.off = next_ptr;
      tmp_node_addr->persist();

      // insert the new node by using PMwCAS
      auto *desc = pmwcas_desc_pool_->Get();
      desc->AddPMwCASTarget(next_addr, next_ptr, new_ptr);
      desc->AddPMwCASTarget(tmp_addr, new_ptr, kNullPtr);  // to prevent memory leak
      if (desc->PMwCAS()) break;

      // if failed, retry from getting the next node
      next_ptr = PMwCASDescriptor::Read<uint64_t>(next_addr, std::memory_order_relaxed);
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
    [[maybe_unused]] const auto &gc_guard = gc_->CreateEpochGuard();

    // prepare a temporary region for a node to be removed
    auto *tmp_node_addr = ReserveNodeAddress();
    *tmp_node_addr = PMEMoid{pool_uuid_, kNullPtr};
    pool_.persist(tmp_node_addr, sizeof(PMEMoid));

    // remove the head node by using PMwCAS
    auto *tmp_addr = &(tmp_node_addr->raw_ptr()->off);
    auto *head_addr = &(root_->head.off);
    while (true) {
      // check this queue has any element
      const auto old_ptr = PMwCASDescriptor::Read<uint64_t>(head_addr, std::memory_order_relaxed);
      ::pmem::obj::persistent_ptr<Node_t> old_head{PMEMoid{pool_uuid_, old_ptr}};
      if (old_head == nullptr) return std::nullopt;

      // prepare PMwCAS targets
      auto *next_addr = &(old_head->next.off);
      const auto new_ptr = PMwCASDescriptor::Read<uint64_t>(next_addr, std::memory_order_relaxed);
      auto *desc = pmwcas_desc_pool_->Get();
      desc->AddPMwCASTarget(head_addr, old_ptr, new_ptr);
      desc->AddPMwCASTarget(next_addr, new_ptr, new_ptr);
      desc->AddPMwCASTarget(tmp_addr, kNullPtr, old_ptr);  // to prevent memory leak

      if (desc->PMwCAS()) {
        // NOTE: this procedure cannot guarantee fault tolerance
        ::pmem::obj::persistent_ptr<Node_t> dummy{PMEMoid{pool_uuid_, old_ptr}};
        gc_->AddGarbage<NodeTarget_t>(dummy.raw_ptr(), pool_);
        return old_head->value;
      }
    }
  }

 private:
  /*####################################################################################
   * Internal constants
   *##################################################################################*/

  /// @brief The layout name for this class.
  static constexpr char kQueueLayout[] = "priority_queue_pmwcas";

  /// @brief The layout name for this class.
  static constexpr char kPMwCASLayout[] = "pmwcas_for_priority_queue";

  /*####################################################################################
   * Internal classes
   *##################################################################################*/

  /**
   * @brief A root class for ::pmem::obj::pool.
   *
   */
  struct Root {
    /// @brief The head of the queue.
    PMEMoid head{OID_NULL};

    /// @brief The head of a linked list for GC.
    ::pmem::obj::persistent_ptr<GarbageNode_t> gc_head{nullptr};

    /// @brief A temporal region to be stored node pointers.
    ::pmem::obj::persistent_ptr<Node_t> tmp_nodes[kMaxThreadNum]{};
  };

  /*####################################################################################
   * Internal utilities for initialization and recovery
   *##################################################################################*/

  /**
   * @brief Create a new pool for a persistent queue.
   *
   * @param pool_path the path of a pool.
   */
  void
  CreateQueuePool(const std::string &pool_path)
  {
    try {
      constexpr size_t kSize = PMEMOBJ_MIN_POOL * 256;  // 2GB
      pool_ = ::pmem::obj::pool<Root>::create(pool_path, kQueueLayout, kSize, CREATE_MODE_RW);
      root_ = pool_.root();
      pool_uuid_ = root_.raw().pool_uuid_lo;

      // create a dummy node
      ::pmem::obj::flat_transaction::run(pool_, [&] {  //
        pmemobj_tx_add_range(root_.raw(), 0, sizeof(PMEMoid));
        root_->head.pool_uuid_lo = pool_uuid_;
      });
    } catch (const std::exception &e) {
      std::cerr << e.what() << std::endl;
      exit(EXIT_FAILURE);
    }
  }

  /**
   * @brief Open and recover the pool of a persistent queue.
   *
   * @param pool_path the path of a pool.
   */
  void
  RecoverQueuePool(const std::string &pool_path)
  {
    try {
      pool_ = ::pmem::obj::pool<Root>::open(pool_path, kQueueLayout);
      root_ = pool_.root();
      pool_uuid_ = root_.raw().pool_uuid_lo;

      // release temporary nodes if exist due to power failure
      for (size_t i = 0; i < kMaxThreadNum; ++i) {
        auto &&tmp_node = root_->tmp_nodes[i];
        if (tmp_node != nullptr) {
          ::pmem::obj::flat_transaction::run(pool_, [&] {  //
            ::pmem::obj::delete_persistent<Node_t>(tmp_node);
          });
        }
      }
    } catch (const std::exception &e) {
      std::cerr << e.what() << std::endl;
      exit(EXIT_FAILURE);
    }
  }

  /*####################################################################################
   * Internal utility functions
   *##################################################################################*/

  /**
   * @return the reserved address for node pointers.
   */
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
            ptr = std::make_unique<ElementHolder_t>(i, reserve_arr_, &(root_->tmp_nodes[i]));
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

  /// @brief A pool for node objects on persistent memopry.
  ::pmem::obj::pool<Root> pool_{};

  /// @brief A root pointer in the pool.
  ::pmem::obj::persistent_ptr<Root> root_{nullptr};

  /// @brief The UUID of the pool.
  size_t pool_uuid_{kNullPtr};

  /// @brief A garbage collector for nodes.
  std::unique_ptr<GC_t> gc_{nullptr};

  /// @brief The pool of PMwCAS descriptors.
  std::unique_ptr<DescriptorPool> pmwcas_desc_pool_{nullptr};

  /// @brief An array representing the occupied state of the descriptor pool
  std::shared_ptr<std::atomic_bool[]> reserve_arr_{new std::atomic_bool[kMaxThreadNum]};
};

#endif  // PMWCAS_BENCHMARK_QUEUE_PRIORITY_QUEUE_PMWCAS_HPP
