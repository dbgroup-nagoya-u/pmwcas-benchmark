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

// the corresponding header
#include "operation_engine.hpp"

// C++ standard libraries
#include <cstddef>

// external libraries
#include "gtest/gtest.h"

class OperationEngineFixture : public ::testing::Test
{
 protected:
  /*############################################################################
   * Constants
   *##########################################################################*/

  static constexpr size_t kArrayCapacity = 1E6;

  static constexpr size_t kTargetNum = 2;

  /*############################################################################
   * Setup/Teardown
   *##########################################################################*/

  void
  SetUp() override
  {
  }

  void
  TearDown() override
  {
  }
};

/*------------------------------------------------------------------------------
 * Test definitions
 *----------------------------------------------------------------------------*/

TEST_F(OperationEngineFixture, GenerateCreateSortedTargetPositions)
{
  constexpr auto kSkewParam = 0;
  constexpr auto kRandomSeed = 0;
  constexpr auto kN = 1000;

  OperationEngine ops_engine{kTargetNum, kArrayCapacity, kSkewParam, kRandomSeed};

  const auto &operations = ops_engine.Generate(kN, kRandomSeed);
  for (const auto &ops : operations) {
    const auto &positions = ops.GetPositions();
    auto prev_pos = positions.at(0);
    for (size_t i = 1; i < kTargetNum; ++i) {
      const auto cur_pos = positions.at(i);
      EXPECT_LT(prev_pos, cur_pos);
      prev_pos = cur_pos;
    }
  }
}
