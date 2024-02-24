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
#include "array/pmwcas_target.hpp"

// C++ standard libraries
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

// external sources
#include "gtest/gtest.h"

// macros for modifying input strings
#define DBGROUP_ADD_QUOTES_INNER(x) #x                     // NOLINT
#define DBGROUP_ADD_QUOTES(x) DBGROUP_ADD_QUOTES_INNER(x)  // NOLINT

/*##############################################################################
 * Global contants
 *############################################################################*/

constexpr size_t kArrayCapacity = PMWCAS_BENCH_MAX_TARGET_NUM;

constexpr size_t kTestThreadNum = DBGROUP_TEST_THREAD_NUM;

constexpr std::string_view kTmpPMEMPath = DBGROUP_ADD_QUOTES(DBGROUP_TEST_TMP_PMEM_PATH);

constexpr size_t kExecNum = 1E5;

const std::string_view use_name = std::getenv("USER");

/*##############################################################################
 * Fixture definitions
 *############################################################################*/

class TmpDirManager : public ::testing::Environment
{
 public:
  void
  SetUp() override
  {
    // check the specified path
    if (kTmpPMEMPath.empty() || !std::filesystem::exists(kTmpPMEMPath)) {
      std::cerr << "WARN: The correct path to persistent memory is not set." << std::endl;
      GTEST_SKIP();
    }
  }

  void
  TearDown() override
  {
  }
};
auto *const env = testing::AddGlobalTestEnvironment(new TmpDirManager);

template <class Competitor>
class PMwCASTargetFixture : public ::testing::Test
{
  using PMwCASTarget_t = PMwCASTarget<Competitor>;

 protected:
  /*############################################################################
   * Setup/Teardown
   *##########################################################################*/

  void
  SetUp() override
  {
    std::filesystem::path pool_path{kTmpPMEMPath};
    pool_path /= use_name;
    target_ = std::make_unique<PMwCASTarget_t>(pool_path, kArrayCapacity);

    ready_num_ = 0;
    ready_for_testing_ = false;
  }

  void
  TearDown() override
  {
    target_ = nullptr;
  }

  /*############################################################################
   * Utilities
   *##########################################################################*/

  void
  RunPMwCAS(  //
      const size_t thread_num,
      const size_t target_num)
  {
    if constexpr (std::is_same_v<Competitor, PCAS>) {
      if (target_num > 1) GTEST_SKIP();
    }

    Operation ops{};
    for (size_t i = 0; i < target_num; ++i) {
      ops.SetPositionIfUnique(i);
    }

    // create worker threads
    std::vector<std::thread> threads{};
    for (size_t i = 0; i < thread_num; ++i) {
      threads.emplace_back([&]() {
        {
          std::unique_lock lock{mtx_};
          ++ready_num_;
          cond_.wait(lock, [this] { return ready_for_testing_; });
        }
        for (size_t i = 0; i < kExecNum; ++i) {
          target_->Execute(ops);
        }
      });
    }

    {  // run the worker threads
      while (ready_num_ < thread_num) {
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
      }
      std::lock_guard x_guard{mtx_};
      ready_for_testing_ = true;
      cond_.notify_all();
    }

    // wait for all the workers to finish
    for (auto &&t : threads) t.join();

    // check validity
    for (size_t i = 0; i < target_num; ++i) {
      const auto v = target_->GetValue(i);
      EXPECT_EQ(kExecNum * thread_num, v);
    }
  }

  /*############################################################################
   * Internal member variables
   *##########################################################################*/

  std::unique_ptr<PMwCASTarget_t> target_{nullptr};

  std::atomic_size_t ready_num_{0};

  std::mutex mtx_{};

  std::condition_variable cond_{};

  bool ready_for_testing_{false};
};

/*##############################################################################
 * Preparation for typed testing
 *############################################################################*/

using TestTargets = ::testing::Types<PMwCAS, MicrosoftPMwCAS, PCAS>;
TYPED_TEST_SUITE(PMwCASTargetFixture, TestTargets);

/*------------------------------------------------------------------------------
 * Test definitions
 *----------------------------------------------------------------------------*/

TYPED_TEST(PMwCASTargetFixture, P1wCASWithSingleThread)
{  //
  TestFixture::RunPMwCAS(1, 1);
}

TYPED_TEST(PMwCASTargetFixture, P3wCASWithSingleThread)
{  //
  TestFixture::RunPMwCAS(1, 3);
}

TYPED_TEST(PMwCASTargetFixture, P1wCASWithMultiThreads)
{  //
  TestFixture::RunPMwCAS(kTestThreadNum, 1);
}

TYPED_TEST(PMwCASTargetFixture, P3wCASWithMultiThreads)
{  //
  TestFixture::RunPMwCAS(kTestThreadNum, 3);
}
