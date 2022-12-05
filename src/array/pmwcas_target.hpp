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

  PMwCASTarget(PmemArray &target_arr) : pool_{target_arr.GetPool()}, root_{pool_.root()} {}

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
  }

  void
  TearDownForWorker()
  {
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

  ::pmem::obj::pool<Root> pool_{};

  ::pmem::obj::persistent_ptr<Root> root_{};
};

/*######################################################################################
 * Specializations for each PMwCAS implementations
 *####################################################################################*/

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

#endif  // PMWCAS_BENCHMARK_ARRAY_PMWCAS_TARGET_HPP
