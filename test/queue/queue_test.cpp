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

// C++ standard libraries
#include <iostream>
#include <random>

// external sources
#include "gtest/gtest.h"

// local sources
#include "common.hpp"
#include "queue/queue_lock.hpp"
#include "queue/queue_microsoft_pmwcas.hpp"
#include "queue/queue_pmwcas.hpp"

namespace dbgroup::test
{
/**
 * @brief A class for managing unit tests.
 *
 */
template <class Queue>
class PmemQueueFixture : public ::testing::Test
{
 protected:
  /*####################################################################################
   * Internal constants
   *##################################################################################*/

  static constexpr auto kLoopNum = 1E4;

  /*####################################################################################
   * Setup/Teardown
   *##################################################################################*/

  void
  SetUp() override
  {
    // create a user directory for testing
    const std::string user_name{std::getenv("USER")};
    pool_path_ /= user_name;
    pool_path_ /= "pmwcas_bench_queue_test";
    std::filesystem::remove_all(pool_path_);
    std::filesystem::create_directories(pool_path_);

    // prepare a queue
    queue_ = std::make_unique<Queue>(pool_path_);
  }

  void
  TearDown() override
  {
    queue_ = nullptr;
    std::filesystem::remove_all(pool_path_);
  }

  /*####################################################################################
   * Internal utility functions
   *##################################################################################*/

  /*####################################################################################
   * Internal test definitions
   *##################################################################################*/

  void
  PopEmptyQueue()
  {
    const auto &value = queue_->Pop();
    EXPECT_FALSE(value);
  }

  void
  PushAndThenPop()
  {
    std::vector<uint64_t> temp;
    std::mt19937_64 rand_engine{std::random_device{}()};
    std::uniform_int_distribution<uint64_t> uni_dist{};
    for (size_t i = 0; i < kLoopNum; ++i) {
      uint64_t value = uni_dist(rand_engine);
      queue_->Push(value);
      temp.push_back(value);
    }
    for (size_t i = 0; i < kLoopNum; ++i) {
      const auto &value = queue_->Pop();
      ASSERT_TRUE(value);
      EXPECT_EQ(*value, temp[i]);
    }
  }

  /*####################################################################################
   * Internal member variables
   *##################################################################################*/

  std::filesystem::path pool_path_{kTmpPMEMPath};

  std::unique_ptr<Queue> queue_{nullptr};
};

/*######################################################################################
 * Preparation for typed testing
 *####################################################################################*/

using TestTargets = ::testing::Types<  //
    QueueWithLock<uint64_t>,
    QueueWithPMwCAS<uint64_t>  //,  //
    // QueueWithMicrosoftPMwCAS<uint64_t>  //
    >;
TYPED_TEST_SUITE(PmemQueueFixture, TestTargets);

/*######################################################################################
 * Test definitions
 *####################################################################################*/

TYPED_TEST(PmemQueueFixture, PopEmptyQueue) { TestFixture::PopEmptyQueue(); }

TYPED_TEST(PmemQueueFixture, PushAndThenPop) { TestFixture::PushAndThenPop(); }
}  // namespace dbgroup::test
