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

#ifndef PMWCAS_BENCHMARK_ARRAY_PMEM_ARRAY_HPP
#define PMWCAS_BENCHMARK_ARRAY_PMEM_ARRAY_HPP

// system headers
#include <libpmemobj++/mutex.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

// C++ standard libraries
#include <filesystem>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <vector>

/**
 * @brief A class to deal with MwCAS target data and algorthms.
 *
 * @tparam Implementation A certain implementation of MwCAS algorithms.
 */
class PmemArray
{
 public:
  /*####################################################################################
   * Public classes
   *##################################################################################*/

  struct PmemRoot {
    /// a mutex object for locking.
    ::pmem::obj::mutex mtx{};

    /// a target region of PMwCAS.
    ::pmem::obj::p<uint64_t> arr[kElementNum]{};
  };

  /*####################################################################################
   * Type aliases
   *##################################################################################*/

  using ArrayPool_t = ::pmem::obj::pool<PmemRoot>;

  /*####################################################################################
   * Public constructors/destructors
   *##################################################################################*/

  /**
   * @brief Construct a new PmemArray object.
   *
   * @param path the path of persistent memory for benchmarking.
   */
  PmemArray(const std::string &path)
  {
    try {
      if (std::filesystem::exists(path)) {
        pmem_pool_ = ArrayPool_t::open(path, kArrayBenchLayout);
      } else {
        constexpr size_t kSize = ((sizeof(PmemRoot) / PMEMOBJ_MIN_POOL) + 2) * PMEMOBJ_MIN_POOL;
        pmem_pool_ = ArrayPool_t::create(path, kArrayBenchLayout, kSize, CREATE_MODE_RW);
      }
    } catch (const std::exception &e) {
      std::cerr << e.what() << '\n';
      exit(EXIT_FAILURE);
    }
  }

  /*####################################################################################
   * Public destructors
   *##################################################################################*/

  ~PmemArray() { pmem_pool_.close(); }

  /*####################################################################################
   * Public getters
   *##################################################################################*/

  /**
   * @return the reference to an opened pmemobj_pool object.
   */
  auto
  GetPool()  //
      -> ArrayPool_t &
  {
    return pmem_pool_;
  }

  /*####################################################################################
   * Public utilities
   *##################################################################################*/

  /**
   * @brief Initialize a target array by filling zeros.
   *
   */
  void
  Initialize()
  {
    constexpr size_t kN = 1e4;  // the number of elements to be initialized atomically
    auto root = pmem_pool_.root();

    // restrict the number of elements to be initialized for transactions
    for (size_t offset = 0; offset < kElementNum; offset += kN) {
      const auto size = (offset + kN < kElementNum) ? kN : kElementNum - offset;
      try {
        ::pmem::obj::transaction::run(
            pmem_pool_,
            [&] {
              for (size_t i = 0; i < size; ++i) {
                root->arr[offset + i] = 0;
              }
            },
            root->mtx);
      } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        exit(EXIT_FAILURE);
      }
    }
  }

  /**
   * @brief Show current values in a target arrays on stdout.
   *
   */
  void
  ShowSampledElements()
  {
    constexpr size_t kSampleNum = 100;
    std::mt19937_64 rand_engine{std::random_device{}()};
    std::uniform_int_distribution<size_t> uni_dist{0, kElementNum - 1};
    auto root = pmem_pool_.root();

    try {
      ::pmem::obj::transaction::run(
          pmem_pool_,
          [&] {
            for (size_t i = 0; i < kSampleNum; ++i) {
              const auto pos = uni_dist(rand_engine);
              const auto &val = root->arr[pos].get_ro();
              std::cout << pos << ": " << val << std::endl;
            }
          },
          root->mtx);
    } catch (const std::exception &e) {
      std::cerr << e.what() << std::endl;
      exit(EXIT_FAILURE);
    }
  }

 private:
  /*####################################################################################
   * Internal member variables
   *##################################################################################*/

  /// a pmemobj_pool object that contains a target array.
  ArrayPool_t pmem_pool_{};
};

#endif  // PMWCAS_BENCHMARK_ARRAY_PMEM_ARRAY_HPP
