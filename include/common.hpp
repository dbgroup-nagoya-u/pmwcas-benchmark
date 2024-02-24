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

// system headers
#include <sys/stat.h>

// C++ standard libraries
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>

/*##############################################################################
 * Global constants and enums
 *############################################################################*/

/// @brief File permission for pmemobj_pool.
constexpr auto kModeRW = S_IRUSR | S_IWUSR;  // NOLINT

/// @brief An alias of std::memory_order_relaxed.
constexpr std::memory_order kMORelax = std::memory_order_relaxed;

/// @brief A flag for removing command line options.
constexpr bool kRemoveParsedFlags = true;

/*##############################################################################
 * Global utilities
 *############################################################################*/

/**
 * @param pmem_dir_str The path to a workspace directory on persistent memory.
 * @param layout A layout name for pmemobj_pool.
 * @return A file path for a given layout.
 */
inline auto
GetPath(  //
    const std::string &pmem_dir_str,
    const std::string &layout)  //
    -> std::string
{
  std::filesystem::path pmem_dir_path{pmem_dir_str};
  return pmem_dir_path.append(layout).native();
}

#endif  // PMWCAS_BENCHMARK_COMMON_HPP
