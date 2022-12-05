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
#include "array/operation.hpp"

// external sources
#include "gtest/gtest.h"

class OperationFixture : public ::testing::Test
{
 protected:
  /*####################################################################################
   * Setup/Teardown
   *##################################################################################*/

  void
  SetUp() override
  {
  }

  void
  TearDown() override
  {
  }
};

/*--------------------------------------------------------------------------------------
 * Test definitions
 *------------------------------------------------------------------------------------*/

TEST_F(OperationFixture, SetPositionIfUniqueWithUniquePositionsSucceed)
{
  Operation ops{};

  for (size_t i = 0; i < kTargetNum; ++i) {
    EXPECT_TRUE(ops.SetPositionIfUnique(i, i));
  }

  for (size_t i = 0; i < kTargetNum; ++i) {
    EXPECT_EQ(ops.GetPosition(i), i);
  }
}

TEST_F(OperationFixture, SetPositionIfUniqueWithDuplicatePositionsFail)
{
  Operation ops{};

  if constexpr (kTargetNum > 1) {
    ops.SetPositionIfUnique(0, 0);
    EXPECT_FALSE(ops.SetPositionIfUnique(1, 0));
  }
}

TEST_F(OperationFixture, SortTargetsWithUniquePositionsSortInAscendingOrder)
{
  Operation ops{};

  for (int64_t i = kTargetNum - 1; i >= 0; --i) {
    const size_t pos = i;
    ops.SetPositionIfUnique(pos, pos);
  }
  ops.SortTargets();

  for (size_t i = 0; i < kTargetNum; ++i) {
    EXPECT_EQ(ops.GetPosition(i), i);
  }
}
