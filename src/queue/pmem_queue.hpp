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
class PmemQueue
{
 public:
  /*####################################################################################
   * Type aliases
   *##################################################################################*/

  using Pool_t = ::pmem::obj::pool<int64_t>;

  /*####################################################################################
   * Public constructors/destructors
   *##################################################################################*/

  /**
   * @brief Construct a new PmemQueue object.
   *
   * @param path the path of persistent memory for benchmarking.
   */
  PmemQueue(const std::string &pmem_dir_str)
  {
    const auto &pmem_queue_path = GetPath(pmem_dir_str, kQueueLayout);
    try {
      if (std::filesystem::exists(pmem_queue_path)) {
        pool = Pool_t::open(pmem_queue_path, kQueueLayout);
      } else {
        constexpr size_t kSize = ((sizeof(int64_t) / PMEMOBJ_MIN_POOL) + 2) * PMEMOBJ_MIN_POOL;
        pool = Pool_t::create(pmem_queue_path, kQueueLayout, kSize, CREATE_MODE_RW);
      }
    } catch (const std::exception &e) {
      std::cerr << e.what() << '\n';
      exit(EXIT_FAILURE);
    }
  }

  /*####################################################################################
   * Public destructors
   *##################################################################################*/

  ~PmemQueue() { pool.close(); }

  /*####################################################################################
   * Public utilities
   *##################################################################################*/

  /**
   * @brief Inserts a new element at the end of the queue.
   *
   */
  void
  push(int64_t value)
  {
    ::pmem::obj::transaction::run(pool, [this, &value] {
      auto n = ::pmem::obj::make_persistent<Node>(value, nullptr);

      if (head == nullptr) {
        head = n;
        tail = n;
      } else {
        tail->next = n;
        tail = n;
      }
    });
  }

  /**
   * @brief Removes the first element in the queue.
   *
   */
  auto
  pop()
  {
    std::optional<int64_t> ret = std::nullopt;
    ::pmem::obj::transaction::run(pool, [this, &ret] {
      ret = head->value;
      auto n = head->next;

      ::pmem::obj::delete_persistent<Node>(head);
      head = std::move(n);

      if (head == nullptr) {
        tail = nullptr;
      }
    });

    return ret;
  }

 private:
  /*####################################################################################
   * Internal constants
   *##################################################################################*/

  /// @brief The layout name for this class.
  static constexpr char kQueueLayout[] = "queue_lock";

  /*####################################################################################
   * Private classes
   *##################################################################################*/

  /**
   * @brief Internal node definition.
   */
  struct Node {
    /*!
     * @brief Constructor.
     *
     */
    Node(int64_t val, ::pmem::obj::persistent_ptr<Node> n)
        : next(std::move(n)), value(std::move(val))
    {
    }

    // Pointer to the next node
    ::pmem::obj::persistent_ptr<Node> next;
    // Value held by this node
    ::pmem::obj::p<int64_t> value;
  };

  /*####################################################################################
   * Internal member variables
   *##################################################################################*/

  Pool_t pool;

  // The head of the queue
  ::pmem::obj::persistent_ptr<Node> head;
  // The tail of the queue
  ::pmem::obj::persistent_ptr<Node> tail;
};

#endif  // PMWCAS_BENCHMARK_QUEUE_PMEM_QUEUE_HPP
