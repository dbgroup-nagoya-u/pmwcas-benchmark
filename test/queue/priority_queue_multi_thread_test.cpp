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

// a corresponding header of this file
#include <future>
#include <iostream>
#include <random>
#include <thread>

// external sources
#include "gtest/gtest.h"

// local sources
#include "common.hpp"
#include "queue/priority_queue_microsoft_pmwcas.hpp"

namespace dbgroup::test
{
/**
 * @brief A class for managing unit tests.
 *
 */
template <class PriorityQueue>
class PriorityQueueFixture : public ::testing::Test
{
 protected:
  /*####################################################################################
   * Internal constants
   *##################################################################################*/

  static constexpr auto kLoopNum = 1E3;

  /*####################################################################################
   * Setup/Teardown
   *##################################################################################*/

  void
  SetUp() override
  {
    // create a user directory for testing
    const std::string user_name{std::getenv("USER")};
    pool_path_ /= user_name;
    pool_path_ /= "pmwcas_bench_priority_queue_multi_thread_test";
    std::filesystem::remove_all(pool_path_);
    std::filesystem::create_directories(pool_path_);

    // prepare a queue
    queue_ = std::make_unique<PriorityQueue>(pool_path_);
  }

  void
  TearDown() override
  {
    queue_ = nullptr;
    std::filesystem::remove_all(pool_path_);
  }

  /*####################################################################################
   * Internal test definitions
   *##################################################################################*/

  void
  RunMT(std::function<void(void)> f)
  {
    std::vector<std::thread> threads{};
    for (size_t id = 0; id < kThreadNum; ++id) {
      threads.emplace_back(f);
    }
    for (auto &&t : threads) {
      t.join();
    }
  }

  void
  PushAndThenPop()
  {
    auto push = [&]() {
      std::mt19937_64 rand_engine{std::random_device{}()};
      std::uniform_int_distribution<uint64_t> uni_dist{};
      for (size_t i = 0; i < kLoopNum; ++i) {
        queue_->Push(uni_dist(rand_engine));
      }
    };

    auto pop = [&]() {
      auto prev_val = std::numeric_limits<uint64_t>::max();
      while (true) {
        const auto &val = queue_->Pop();
        if (!val) break;

        EXPECT_LE(*val, prev_val);
        prev_val = *val;
      }
    };

    RunMT(push);
    RunMT(pop);
  }

  /*####################################################################################
   * Internal member variables
   *##################################################################################*/

  std::filesystem::path pool_path_{kTmpPMEMPath};

  std::unique_ptr<PriorityQueue> queue_{nullptr};
};

/*######################################################################################
 * Preparation for typed testing
 *####################################################################################*/

using TestTargets = ::testing::Types<           //
    PriorityQueueWithMicrosoftPMwCAS<uint64_t>  //
    >;
TYPED_TEST_SUITE(PriorityQueueFixture, TestTargets);

/*######################################################################################
 * Test definitions
 *####################################################################################*/

TYPED_TEST(PriorityQueueFixture, PushAndThenPop)
{  //
  TestFixture::PushAndThenPop();
}

}  // namespace dbgroup::test
