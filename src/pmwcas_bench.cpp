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
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <random>
#include <string>

// external system libraries
#include <gflags/gflags.h>

// external libraries
#include "benchmark/benchmarker.hpp"

// local sources
#include "competitor.hpp"
#include "operation_engine.hpp"
#include "pmwcas_target.hpp"
#include "validaters.hpp"

/*##############################################################################
 * Options for selecting competitors
 *############################################################################*/

DEFINE_bool(pmwcas, false, "Use our PMwCAS as a competitor.");

DEFINE_bool(microsoft_pmwcas, false, "Use a microsoft/pmwcas as a competitor.");

DEFINE_bool(pcas, false, "Use PCAS as a competitor.");

/*##############################################################################
 * Options for controling workload
 *############################################################################*/

DEFINE_uint64(num_exec, 1000000, "The number of PMwCAS operations executed by each worker.");
DEFINE_validator(num_exec, &ValidateNonZero);

DEFINE_uint64(num_thread, 8, "The number of worker threads for benchmarking.");
DEFINE_validator(num_thread, &ValidateNonZero);

DEFINE_double(skew_parameter, 0, "A skew parameter (based on Zipf's law).");
DEFINE_validator(skew_parameter, &ValidatePositiveVal);

DEFINE_uint64(arr_cap, 1000000, "The capacity of an array for PMwCAS targets.");
DEFINE_validator(arr_cap, &ValidateNonZero);

DEFINE_uint64(block_size, 256, "The size of each memory block.");
DEFINE_validator(block_size, &ValidateBlockSize);

/*##############################################################################
 * Utility options
 *############################################################################*/

DEFINE_string(seed, "", "A random seed for reproducibility.");
DEFINE_validator(seed, &ValidateRandomSeed);

DEFINE_uint64(timeout, 10, "Timeout in seconds.");
DEFINE_validator(timeout, &ValidateNonZero);

DEFINE_bool(csv, false, "Output benchmark results as a CSV format.");

DEFINE_bool(throughput, true, "true: measure throughput, false: measure latency.");

/*##############################################################################
 * Utility functions
 *############################################################################*/

/**
 * @brief Run procedures for benchmarking with a given implementation.
 *
 * @tparam Implementation an implementation to be benchmarked.
 * @param target_name the output name of a implementation.
 * @param pmem_dir_str the path to persistent memory.
 */
template <class Implementation>
void
Run(  //
    const std::string &target_name,
    const std::string &pmem_dir_str,
    const size_t target_num)
{
  using Target_t = PMwCASTarget<Implementation>;
  using Bench_t = ::dbgroup::benchmark::Benchmarker<Target_t, Operation, OperationEngine>;
  constexpr auto kPercentile = "0.01,0.05,0.10,0.20,0.30,0.40,0.50,0.60,0.70,0.80,0.90,0.95,0.99";

  const auto random_seed = (FLAGS_seed.empty()) ? std::random_device{}()  //
                                                : std::stoul(FLAGS_seed);
  Target_t target{pmem_dir_str, FLAGS_arr_cap, FLAGS_block_size};
  OperationEngine ops_engine{target_num, FLAGS_arr_cap, FLAGS_skew_parameter, random_seed};

  Bench_t bench{target,      target_name,      ops_engine, FLAGS_num_exec, FLAGS_num_thread,
                random_seed, FLAGS_throughput, FLAGS_csv,  FLAGS_timeout,  kPercentile};
  bench.Run();
}

/*##############################################################################
 * Main procedure
 *############################################################################*/

auto
main(  //
    int argc,
    char *argv[])  //
    -> int
{
  // parse command line options
  constexpr bool kRemoveParsedFlags = true;
  gflags::SetUsageMessage("measures throughput/latency of PMwCAS implementations.");
  gflags::ParseCommandLineFlags(&argc, &argv, kRemoveParsedFlags);

  // parse command line arguments
  if (argc < 3) {
    std::cerr << "Usage: ./pmwcas_bench --<competitor> <path_to_pmem_dir> <target_word_num>\n";
    return 1;
  }
  const std::string pmem_dir_str{argv[1]};
  if (!std::filesystem::exists(pmem_dir_str) || !std::filesystem::is_directory(pmem_dir_str)) {
    std::cerr << "[Error] The given path does not specify a directory.\n";
    return 1;
  }
  const auto target_num = std::stoull(argv[2]);
  constexpr auto kMax = ::dbgroup::pmem::atomic::kPMwCASCapacity;
  if (target_num > kMax) {
    std::cerr << "[Error] The current benchmark can swap up to " << kMax << " words.\n";
    return 1;
  }

  // run benchmark for each implementaton
  if (FLAGS_pmwcas) {
    Run<PMwCAS>("PMwCAS", pmem_dir_str, target_num);
  }
  if (FLAGS_microsoft_pmwcas) {
    Run<MicrosoftPMwCAS>("microsoft/pmwcas", pmem_dir_str, target_num);
  }
  if (FLAGS_pcas) {
    if (target_num > 1) {
      throw std::runtime_error{"PCAS cannot deal with multi-word swapping."};
    }
    Run<PCAS>("PCAS", pmem_dir_str, target_num);
  }

  return 0;
}
