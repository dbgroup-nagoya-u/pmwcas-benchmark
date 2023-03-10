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
#include <string>

// external system libraries
#include <gflags/gflags.h>

// external libraries
#include "benchmark/benchmarker.hpp"

// local sources
#include "clo_validaters.hpp"
#include "queue/bench_target.hpp"
#include "queue/operation_engine.hpp"

// local sources
#include "queue/priority_queue_microsoft_pmwcas.hpp"
#include "queue/queue_lock.hpp"
#include "queue/queue_microsoft_pmwcas.hpp"
#include "queue/queue_pmwcas.hpp"

/*######################################################################################
 * Type aliases for competitors
 *####################################################################################*/

/// an alias for lock based implementations.
using Lock = QueueWithLock<uint64_t>;

/// an alias for our PMwCAS based implementations.
using QueuePMwCAS = QueueWithPMwCAS<uint64_t>;

/// an alias for microsoft/pmwcas based implementations.
using QueueMicrosoftPMwCAS = QueueWithMicrosoftPMwCAS<uint64_t>;
using PriorityQueueMicrosoftPMwCAS = PriorityQueueWithMicrosoftPMwCAS<uint64_t>;

/*######################################################################################
 * Command-line options
 *####################################################################################*/

DEFINE_uint64(num_exec, 1000000, "The number of PMwCAS operations for each worker.");
DEFINE_uint64(num_thread, 8, "The number of worker threads for benchmarking.");
DEFINE_string(seed, "", "A random seed to control reproducibility.");
DEFINE_uint64(timeout, 10, "Seconds to timeout");
DEFINE_bool(csv, false, "Output benchmark results as CSV format.");
DEFINE_bool(throughput, true, "true: measure throughput, false: measure latency.");
DEFINE_bool(use_priority_queue, false, "Use priority queues for benchmarks.");
DEFINE_bool(pmwcas, false, "Use our PMwCAS as a benchmark target.");
DEFINE_bool(lock, false, "Use an exclusive lock as a benchmark target.");
DEFINE_bool(microsoft_pmwcas, false, "Use a microsoft/pmwcas as a benchmark target.");

DEFINE_validator(num_exec, &ValidateNonZero);
DEFINE_validator(num_thread, &ValidateNonZero);
DEFINE_validator(seed, &ValidateRandomSeed);

/*######################################################################################
 * Utility functions
 *####################################################################################*/

/**
 * @brief Run procedures for benchmarking with a given implementation.
 *
 * @tparam Queue an implementation to be benchmarked.
 * @param target_name the output name of a implementation.
 * @param pmem_dir_str the path to persistent memory.
 */
template <class Queue>
void
Run(  //
    const std::string &target_name,
    const std::string &pmem_dir_str)
{
  using Target_t = BenchTarget<Queue>;
  using Operation = std::optional<uint64_t>;
  using Bench_t = ::dbgroup::benchmark::Benchmarker<Target_t, Operation, OperationEngine>;

  const auto random_seed = (FLAGS_seed.empty()) ? std::random_device{}() : std::stoul(FLAGS_seed);

  {  // initialize a persistent queue with a thousand elements
    std::mt19937_64 rand_engine{random_seed};
    std::uniform_int_distribution<uint64_t> uni_dist{};
    Queue queue{pmem_dir_str};
    while (queue.Pop()) {
      // remove all the elements
    }
    for (size_t i = 0; i < 1E3; ++i) {
      queue.Push(uni_dist(rand_engine));
    }
  }

  // run benchmark
  Target_t target{pmem_dir_str};
  OperationEngine ops_engine{};
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
  gflags::SetUsageMessage("measures throughput/latency of persistent queues.");
  gflags::ParseCommandLineFlags(&argc, &argv, kRemoveParsedFlags);

  if (argc < 2) {
    std::cerr << "NOTE: Specify a path to be stored a persistent queue." << std::endl;
    return 0;
  }

  const std::string pmem_dir_str{argv[1]};

  // run benchmark for each implementation

  if (FLAGS_lock) {
    Run<Lock>("Global Lock", pmem_dir_str);
  }
  if (FLAGS_pmwcas) {
    Run<QueuePMwCAS>("pmwcas: queue", pmem_dir_str);
  }
  if (FLAGS_microsoft_pmwcas) {
    if (FLAGS_use_priority_queue) {
      Run<PriorityQueueMicrosoftPMwCAS>("microsoft/pmwcas: priority queue", pmem_dir_str);
    } else {
      Run<QueueMicrosoftPMwCAS>("microsoft/pmwcas: queue", pmem_dir_str);
    }
  }

  return 0;
}
