/*
 * Copyright 2021 Database Group, Nagoya University
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

#include "operation.hpp"

#include <vector>

#include "gtest/gtest.h"

class OperationFixture : public ::testing::Test
{
 protected:
  /*################################################################################################
   * Setup/Teardown
   *##############################################################################################*/

  void
  SetUp() override
  {
    for (size_t i = 0; i < kTargetNum; ++i) {
      addresses.emplace_back(new uint64_t{0});
    }
  }

  void
  TearDown() override
  {
    for (auto &&addr : addresses) {
      delete addr;
    }
  }

  /*################################################################################################
   * Member variables
   *##############################################################################################*/

  std::vector<uint64_t *> addresses;
};

/*--------------------------------------------------------------------------------------------------
 * Test definitions
 *------------------------------------------------------------------------------------------------*/

TEST_F(OperationFixture, SetAddr_UniqueAddresses_CanGetSetAddresses)
{
  Operation ops{};

  for (size_t i = 0; i < kTargetNum; ++i) {
    EXPECT_TRUE(ops.SetAddr(i, addresses[i]));
  }

  for (size_t i = 0; i < kTargetNum; ++i) {
    const auto addr = ops.GetAddr(i);
    EXPECT_EQ(addresses[i], addr);
    EXPECT_EQ(0, *addr);
  }
}

TEST_F(OperationFixture, SetAddr_DuplicateAddress_SetAddressFail)
{
  Operation ops{};

  if constexpr (kTargetNum > 1) {
    ops.SetAddr(0, addresses[0]);
    EXPECT_FALSE(ops.SetAddr(1, addresses[0]));
  }
}

TEST_F(OperationFixture, SortTargets_UniqueAddress_AddressesSorted)
{
  Operation ops{};

  for (size_t i = 0; i < kTargetNum; ++i) {
    ops.SetAddr(i, addresses[i]);
  }
  ops.SortTargets();

  auto prev_addr = ops.GetAddr(0);
  for (size_t i = 1; i < kTargetNum; ++i) {
    const auto addr = ops.GetAddr(i);
    EXPECT_LT(prev_addr, addr);
    prev_addr = addr;
  }
}
