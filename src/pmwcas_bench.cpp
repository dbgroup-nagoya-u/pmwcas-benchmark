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

#include <gflags/gflags.h>

#include <string>

#include "benchmark/benchmarker.hpp"
#include "mwcas_target.hpp"
#include "operation_engine.hpp"

/*##################################################################################################
 * CLI validators
 *################################################################################################*/

template <class Number>
static bool
ValidatePositiveVal(const char *flagname, const Number value)
{
  if (value >= 0) {
    return true;
  }
  std::cout << "A value must be positive for " << flagname << std::endl;
  return false;
}

template <class Number>
static bool
ValidateNonZero(const char *flagname, const Number value)
{
  if (value != 0) {
    return true;
  }
  std::cout << "A value must be not zero for " << flagname << std::endl;
  return false;
}

static bool
ValidateRandomSeed([[maybe_unused]] const char *flagname, const std::string &seed)
{
  if (seed.empty()) {
    return true;
  }

  for (size_t i = 0; i < seed.size(); ++i) {
    if (!std::isdigit(seed[i])) {
      std::cout << "A random seed must be unsigned integer type" << std::endl;
      return false;
    }
  }
  return true;
}

/*##################################################################################################
 * CLI arguments
 *################################################################################################*/

DEFINE_uint64(num_field, 1000000, "The total number of target fields");
DEFINE_validator(num_field, &ValidateNonZero);
DEFINE_uint64(num_exec, 10000000, "The total number of MwCAS operations");
DEFINE_validator(num_exec, &ValidateNonZero);
DEFINE_uint64(num_thread, 8, "The number of worker threads for benchmarking");
DEFINE_validator(num_thread, &ValidateNonZero);
DEFINE_double(skew_parameter, 0, "A skew parameter (based on Zipf's law)");
DEFINE_validator(skew_parameter, &ValidatePositiveVal);
DEFINE_uint64(num_init_thread, 8, "The number of worker threads for initialization");
DEFINE_validator(num_init_thread, &ValidateNonZero);
DEFINE_string(seed, "", "A random seed to control reproducibility");
DEFINE_validator(seed, &ValidateRandomSeed);
DEFINE_bool(csv, false, "Output benchmark results as CSV format");
DEFINE_bool(throughput, true, "true: measure throughput, false: measure latency");
DEFINE_bool(mwcas, true, "Use our MwCAS library as a benchmark target");
DEFINE_bool(pmwcas, true, "Use the PMwCAS library as a benchmark target");
DEFINE_bool(aopt, true, "Use AOPT library as a benchmark target");
DEFINE_bool(single, false, "Use Single CAS as a benchmark target");

/*##################################################################################################
 * Utility functions
 *################################################################################################*/

template <class Implementation>
void
RunBenchmark(const std::string &target_name)
{
  using MwCASTarget_t = MwCASTarget<Implementation>;
  using Bench_t = ::dbgroup::benchmark::Benchmarker<MwCASTarget_t, Operation, OperationEngine>;

  if constexpr (std::is_same_v<Implementation, AOPT>) {
    AOPT::StartGC(100000, 4);
  }

  MwCASTarget_t target{FLAGS_num_field, FLAGS_num_init_thread, FLAGS_num_thread};
  OperationEngine ops_engine{target.ReferTargetFields(), FLAGS_skew_parameter};
  const auto random_seed = (FLAGS_seed.empty()) ? std::random_device{}() : std::stoul(FLAGS_seed);

  Bench_t bench{target,      ops_engine,       FLAGS_num_exec, FLAGS_num_thread,
                random_seed, FLAGS_throughput, FLAGS_csv,      target_name};
  bench.Run();

  if constexpr (std::is_same_v<Implementation, AOPT>) {
    AOPT::StopGC();
  }
}

/*##################################################################################################
 * Main function
 *################################################################################################*/

int
main(int argc, char *argv[])
{
  // parse command line options
  gflags::SetUsageMessage("measures throughput/latency of MwCAS implementations.");
  gflags::ParseCommandLineFlags(&argc, &argv, false);

  // run benchmark for each implementaton
  if (FLAGS_mwcas) RunBenchmark<MwCAS>("MwCAS without GC");
  if (FLAGS_pmwcas) RunBenchmark<PMwCAS>("PMwCAS");
  if (FLAGS_aopt) RunBenchmark<AOPT>("AOPT");
  if (FLAGS_single) RunBenchmark<SingleCAS>("Single CAS");

  return 0;
}
