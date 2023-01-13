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

// system headers
#include <gflags/gflags.h>

// C++ standard libraries
#include <string>

// external sources
#include "benchmark/benchmarker.hpp"

// local sources
#include "array/operation_engine.hpp"
#include "array/pmem_array.hpp"
#include "clo_validaters.hpp"

// add target after defining competitors
#include "array/pmwcas_target.hpp"

/*######################################################################################
 * Command-line options
 *####################################################################################*/

DEFINE_uint64(num_exec, 1000000, "The number of PMwCAS operations for each worker.");
DEFINE_uint64(num_thread, 8, "The number of worker threads for benchmarking.");
DEFINE_double(skew_parameter, 0, "A skew parameter (based on Zipf's law).");
DEFINE_string(seed, "", "A random seed to control reproducibility.");
DEFINE_uint64(timeout, 10, "Seconds to timeout");
DEFINE_bool(csv, false, "Output benchmark results as CSV format.");
DEFINE_bool(throughput, true, "true: measure throughput, false: measure latency.");
DEFINE_bool(init, false, "Initialize all the array elemnts with zeros and exit.");
DEFINE_bool(show, false, "Show random-sampled elemnts in the array and exit.");
DEFINE_bool(pmwcas, false, "Use our PMwCAS as a benchmark target.");
DEFINE_bool(lock, false, "Use an exclusive lock as a benchmark target.");
DEFINE_bool(microsoft_pmwcas, false, "Use a microsoft/pmwcas as a benchmark target.");

DEFINE_validator(num_exec, &ValidateNonZero);
DEFINE_validator(num_thread, &ValidateNonZero);
DEFINE_validator(skew_parameter, &ValidatePositiveVal);
DEFINE_validator(seed, &ValidateRandomSeed);

/*######################################################################################
 * Utility functions
 *####################################################################################*/

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
    const std::string &pmem_dir_str)
{
  using Target_t = PMwCASTarget<Implementation>;
  using Bench_t = ::dbgroup::benchmark::Benchmarker<Target_t, Operation, OperationEngine>;

  const auto random_seed = (FLAGS_seed.empty()) ? std::random_device{}() : std::stoul(FLAGS_seed);
  Target_t target{pmem_dir_str, FLAGS_num_thread};
  OperationEngine ops_engine{FLAGS_skew_parameter, random_seed};

  Bench_t bench{target,      target_name,      ops_engine, FLAGS_num_exec, FLAGS_num_thread,
                random_seed, FLAGS_throughput, FLAGS_csv,  FLAGS_timeout};
  bench.Run();
}

/*######################################################################################
 * Main procedure
 *####################################################################################*/

int
main(int argc, char *argv[])
{
  constexpr auto kRemoveParsedFlags = true;

  // parse command line options
  gflags::SetUsageMessage("measures throughput/latency of PMwCAS implementations.");
  gflags::ParseCommandLineFlags(&argc, &argv, kRemoveParsedFlags);

  if (argc < 2) {
    std::cerr << "Specify a path to be stored a persistent array." << std::endl;
    return 0;
  }

  const std::string pmem_dir_str{argv[1]};

  if (FLAGS_init) {
    std::cout << "Initialize a persitent array with zeros..." << std::endl;
    PmemArray{pmem_dir_str}.Initialize();
    std::cout << "The persitent array is initialized with zeros." << std::endl;
    return 0;
  }

  if (FLAGS_show) {
    PmemArray{pmem_dir_str}.ShowSampledElements();
    return 0;
  }

  // run benchmark for each implementaton

  if (FLAGS_pmwcas) {
    Run<PMwCAS>("PMwCAS", pmem_dir_str);
  }
  if (FLAGS_lock) {
    Run<Lock>("Global Lock", pmem_dir_str);
  }
  if (FLAGS_microsoft_pmwcas) {
    Run<MicrosoftPMwCAS>("microsoft/pmwcas", pmem_dir_str);
  }

  return 0;
}
