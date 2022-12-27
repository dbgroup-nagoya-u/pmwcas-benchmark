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
#include <iostream>

// external sources
#include "gtest/gtest.h"

// local sources
#include "queue/queue_microsoft_pmwcas.hpp"

class QueueFixture : public ::testing::Test
{
 protected:
  /*####################################################################################
   * Setup/Teardown
   *##################################################################################*/

  void
  SetUp() override
  {
    // create a user directory for testing
    const std::string user_name{std::getenv("USER")};
    pool_path_ /= user_name;
    pool_path_ /= "queue_test";
    std::filesystem::remove_all(pool_path_);
    std::filesystem::create_directories(pool_path_);

    // prepare a queue
    queue_ = std::make_unique<QueueWithMicrosoftPMwCAS<uint64_t>>(pool_path_, 1);
  }

  void
  TearDown() override
  {
    queue_ = nullptr;
    std::filesystem::remove_all(pool_path_);
  }

  std::filesystem::path pool_path_{"/pmem_tmp"};

  std::unique_ptr<QueueWithMicrosoftPMwCAS<uint64_t>> queue_{nullptr};
};

/*--------------------------------------------------------------------------------------
 * Test definitions
 *------------------------------------------------------------------------------------*/

TEST_F(QueueFixture, SampleTest)
{
  for (size_t i = 0; i < 10000; ++i) {
    queue_->Push(i);
  }
  for (size_t i = 0; i < 10000; ++i) {
    EXPECT_EQ(queue_->Pop(), i);
  }
}
