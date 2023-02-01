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
#include <thread>

// external sources
#include "gtest/gtest.h"

// local sources
#include "common.hpp"
#include "queue/queue_microsoft_pmwcas.hpp"
#include "queue/queue_pmwcas.hpp"

namespace dbgroup::test
{
/**
 * @brief A class for managing unit tests.
 *
 */
template <class Queue>
class QueueFixture : public ::testing::Test
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
    pool_path_ /= "pmwcas_bench_queue_multi_thread_test";
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

  static auto
  GetCounter()  //
      -> std::array<size_t, kThreadNum>
  {
    std::array<size_t, kThreadNum> counter{};
    counter.fill(0);
    return counter;
  }

  static auto
  MergeCounter(  //
      std::array<size_t, kThreadNum> &base,
      const std::array<size_t, kThreadNum> &merged)
  {
    for (size_t i = 0; i < kThreadNum; ++i) {
      base.at(i) += merged.at(i);
    }
  }

  void
  RunMT(std::function<void(size_t, std::promise<std::array<size_t, kThreadNum>>)> f)
  {
    std::vector<std::future<std::array<size_t, kThreadNum>>> futures{};
    for (size_t id = 0; id < kThreadNum; ++id) {
      std::promise<std::array<size_t, kThreadNum>> p{};
      futures.emplace_back(p.get_future());
      std::thread{f, id, std::move(p)}.detach();
    }

    auto &&counter = GetCounter();
    for (auto &&fut : futures) {
      MergeCounter(counter, fut.get());
    }

    for (const auto count : counter) {
      EXPECT_EQ(count, kLoopNum);
    }
  }

  /*####################################################################################
   * Internal test definitions
   *##################################################################################*/

  void
  PushAndThenPop()
  {
    auto f = [&](const size_t thread_id, std::promise<std::array<size_t, kThreadNum>> p) {
      auto &&counter = GetCounter();

      for (size_t i = 0; i < kLoopNum; ++i) {
        queue_->Push(thread_id);
      }
      for (size_t i = 0; i < kLoopNum; ++i) {
        while (true) {
          const auto &val = queue_->Pop();
          if (val) {
            ++counter.at(*val);
            break;
          }
        }
      }

      p.set_value(std::move(counter));
    };

    RunMT(f);
  }

  void
  PushAndPopConcurrently()
  {
    auto f = [&](const size_t thread_id, std::promise<std::array<size_t, kThreadNum>> p) {
      auto &&counter = GetCounter();

      for (size_t i = 0; i < kLoopNum; ++i) {
        queue_->Push(thread_id);
        while (true) {
          const auto &val = queue_->Pop();
          if (val) {
            ++counter.at(*val);
            break;
          }
        }
      }

      p.set_value(std::move(counter));
    };

    RunMT(f);
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
    QueueWithPMwCAS<uint64_t>
    //  QueueWithMicrosoftPMwCAS<uint64_t>  //
    >;
TYPED_TEST_SUITE(QueueFixture, TestTargets);

/*######################################################################################
 * Test definitions
 *####################################################################################*/

TYPED_TEST(QueueFixture, PushAndThenPop)
{  //
  TestFixture::PushAndThenPop();
}

TYPED_TEST(QueueFixture, PushAndPopConcurrently)
{  //
  TestFixture::PushAndPopConcurrently();
}

}  // namespace dbgroup::test
