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

#ifndef PMWCAS_BENCHMARK_QUEUE_NODE_PMWCAS_HPP
#define PMWCAS_BENCHMARK_QUEUE_NODE_PMWCAS_HPP

// external libraries
#include <libpmemobj++/p.hpp>

// local sources
#include "common.hpp"

/**
 * @brief A class for representing nodes in a linked list.
 *
 * @tparam T a type to be stored.
 */
template <class T>
class Node
{
 public:
  /*##################################################################################
   * Public constructors and assignment operators
   *################################################################################*/

  /**
   * @brief Construct a new Node object.
   *
   * @param val a value to be stored.
   * @param pool_uuid the UUID of the corresponding pool.
   */
  explicit Node(  //
      T val,
      const size_t pool_uuid)
      : value(std::move(val)), next{pool_uuid, kNullPtr}
  {
  }

  Node(const Node &) = delete;
  Node(Node &&) = delete;

  Node &operator=(const Node &) = delete;
  Node &operator=(Node &&) = delete;

  /*##################################################################################
   * Public destructors
   *################################################################################*/

  ~Node() = default;

  /*##################################################################################
   * Public member variables
   *################################################################################*/

  /// @brief The stored value.
  T value{};

  /// @brief The next node in a linked list.
  PMEMoid next{OID_NULL};
};

/**
 * @brief A class for registering Node objects with GC.
 *
 */
template <bool kReuse>
struct NodeTarget {
  // do not call destructor
  using T = void;

  // target pages are on persistent memory
  static constexpr bool kOnPMEM = true;

  // reuse garbage-collected pages
  static constexpr bool kReusePages = kReuse;
};

#endif  // PMWCAS_BENCHMARK_QUEUE_NODE_PMWCAS_HPP
