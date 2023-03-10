/*
 * Copyright 2023 Database Group, Nagoya University
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
#ifndef PMWCAS_BENCHMARK_QUEUE_PMEM_QUEUE_HPP
#define PMWCAS_BENCHMARK_QUEUE_PMEM_QUEUE_HPP

// C++ standard libraries
#include <exception>
#include <iostream>
#include <optional>
#include <sstream>
#include <utility>

// external libraries
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/mutex.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/transaction.hpp>
#include <libpmemobj++/utils.hpp>

// local sources
#include "common.hpp"

/**
 * @brief Persistent memory list-based queue.
 *
 */
template <class T>
class QueueWithLock
{
 public:
  /*####################################################################################
   * Internal classes
   *##################################################################################*/

  /**
   * @brief Internal node definition.
   */
  struct Node {
    /*!
     * @brief Constructor.
     *
     */
    Node(T val, ::pmem::obj::persistent_ptr<Node> n) : next(std::move(n)), value(std::move(val)) {}

    // Pointer to the next node
    ::pmem::obj::persistent_ptr<Node> next;
    // Value held by this node
    ::pmem::obj::p<T> value;
  };

  /**
   * @brief A root class for ::pmem::obj::pool.
   *
   */
  struct Root {
    /// a mutex object for locking.
    ::pmem::obj::mutex mtx{};

    /// @brief The head of the queue.
    ::pmem::obj::persistent_ptr<Node> head{nullptr};

    /// @brief The tail of the queue.
    ::pmem::obj::persistent_ptr<Node> tail{nullptr};
  };

  /*####################################################################################
   * Type aliases
   *##################################################################################*/

  using Pool_t = pmem::obj::pool<Root>;

  /*####################################################################################
   * Public constructors/destructors
   *##################################################################################*/

  /**
   * @brief Construct a new PmemQueue object.
   *
   * @param path the path of persistent memory for benchmarking.
   */
  QueueWithLock(const std::string &pmem_dir_str)
  {
    const auto &pmem_queue_path = GetPath(pmem_dir_str, kQueueLayout);
    try {
      if (std::filesystem::exists(pmem_queue_path)) {
        pool_ = Pool_t::open(pmem_queue_path, kQueueLayout);
      } else {
        constexpr size_t kSize = PMEMOBJ_MIN_POOL * 256;  // 2GB
        pool_ = Pool_t::create(pmem_queue_path, kQueueLayout, kSize, CREATE_MODE_RW);
      }
    } catch (const std::exception &e) {
      std::cerr << e.what() << '\n';
      exit(EXIT_FAILURE);
    }
    root_ = pool_.root();
  }

  /*####################################################################################
   * Public destructors
   *##################################################################################*/

  ~QueueWithLock() { pool_.close(); }

  /*####################################################################################
   * Public utilities
   *##################################################################################*/

  /**
   * @brief Inserts a new element at the end of the queue.
   *
   */
  void
  Push(T value)
  {
    try {
      ::pmem::obj::transaction::run(
          pool_,
          [this, &value] {
            auto &&n = ::pmem::obj::make_persistent<Node>(value, nullptr);
            if (root_->head == nullptr) {
              root_->head = n;
              root_->tail = n;
            } else {
              root_->tail->next = n;
              root_->tail = n;
            }
          },
          root_->mtx);
    } catch (const ::pmem::transaction_error &e) {
      std::cerr << e.what() << std::endl;
      exit(EXIT_FAILURE);
    }
  }

  /**
   * @brief Removes the first element in the queue.
   *
   */
  auto
  Pop()
  {
    try {
      std::optional<T> ret = std::nullopt;
      ::pmem::obj::transaction::run(
          pool_,
          [this, &ret] {
            if (root_->head) {
              ret = root_->head->value;
              auto &&n = root_->head->next;

              ::pmem::obj::delete_persistent<Node>(root_->head);
              root_->head = std::move(n);

              if (root_->head == nullptr) {
                root_->tail = nullptr;
              }
            }
          },
          root_->mtx);

      return ret;
    } catch (const ::pmem::transaction_error &e) {
      std::cerr << e.what() << std::endl;
      exit(EXIT_FAILURE);
    }
  }

 private:
  /*####################################################################################
   * Internal constants
   *##################################################################################*/

  /// @brief The layout name for this class.
  static constexpr char kQueueLayout[] = "queue_lock";

  /*####################################################################################
   * Internal member variables
   *##################################################################################*/
  /// @brief A pool for node objects on persistent memory.
  ::pmem::obj::pool<Root> pool_{};

  /// @brief A root pointer in the pool.
  ::pmem::obj::persistent_ptr<Root> root_{nullptr};

  /// @brief The UUID of the pool.
  size_t pool_uuid_{kNullPtr};
};

#endif  // PMWCAS_BENCHMARK_QUEUE_PMEM_QUEUE_HPP
