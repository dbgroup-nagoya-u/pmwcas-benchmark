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

#ifndef PMWCAS_BENCHMARK_COMMON_HPP
#define PMWCAS_BENCHMARK_COMMON_HPP

// C++ standard libraries
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>

/*######################################################################################
 * Global constants and enums
 *####################################################################################*/

/// a file permission for pmemobj_pool.
#define CREATE_MODE_RW (S_IWUSR | S_IRUSR)

/// @brief The interval of GC threads in micro seconds.
constexpr size_t kGCInterval = 1E5;

/// @brief The number of GC threads.
constexpr size_t kGCThreadNum = 1;

/// @brief The NULL value for PMEMoid.off.
constexpr size_t kNullPtr = 0;

/// @brief A flag for reusing pages on persistent memory.
constexpr bool kReusePageOnPMEM = true;

/*######################################################################################
 * Global utilities
 *####################################################################################*/

/**
 * @param pmem_dir_str the path to persistent memory.
 * @param layout the name of a layout for pmemobj_pool.
 * @return the file path for a given layout.
 */
auto
GetPath(  //
    const std::string &pmem_dir_str,
    const std::string &layout)  //
    -> std::string
{
  std::filesystem::path pmem_dir_path{pmem_dir_str};
  return pmem_dir_path.append(layout).native();
}

#endif  // PMWCAS_BENCHMARK_COMMON_HPP
