#!/bin/bash

set -ue

########################################################################################
# Documents
########################################################################################

BENCH_BIN=""
CONFIG_ENV=""
PMEM_DIR=""
WORKSPACE_DIR=$(cd $(dirname ${BASH_SOURCE:-${0}})/.. && pwd)
MEASURE_THROUGHPUT="t"
USE_PRIORITY_QUEUE="f"

usage() {
  cat 1>&2 << EOS
Usage:
  ${BASH_SOURCE:-${0}} <bench_bin> <config> <pmem_dir> 1> results.csv 2> error.log
Description:
  Run benchmark to measure throughput. All the benchmark results are output in CSV
  format.
Arguments:
  <bench_bin>: A path to a binary file for benchmarking.
  <config>: A path to a configuration file for benchmarking.
  <pmem_dir> : A path to a directory on persistent memory.
Options:
  -h: Show this messsage and exit.
  -n: Only execute benchmark on the CPUs of nodes. See "man numactl" for details.
  -p: Use priority queues for benchmarks (default: false).
  -l: Use latency as a criteria (default: false).
EOS
  exit 1
}

########################################################################################
# Parse options
########################################################################################

while getopts n:plh OPT
do
  case ${OPT} in
    n) NUMA_NODES=${OPTARG}
      ;;
    p) USE_PRIORITY_QUEUE="t"
      ;;
    l) MEASURE_THROUGHPUT="f"
      ;;
    h) usage
      ;;
    \?) usage
      ;;
  esac
done
shift $((${OPTIND} - 1))

########################################################################################
# Parse arguments
########################################################################################

if [ ${#} != 3 ]; then
  usage
fi

BENCH_BIN=${1}
CONFIG_ENV=${2}
PMEM_DIR=${3}

if [ ! -f "${BENCH_BIN}" ]; then
  echo "There is no specified benchmark binary."
  exit 1
fi
if [ ! -f "${CONFIG_ENV}" ]; then
  echo "There is no specified configuration file."
  exit 1
fi
if [ ! -d "${PMEM_DIR}" ]; then
  echo "There is no specified directory."
  exit 1
fi

if [ -n "${NUMA_NODES}" ]; then
  BENCH_BIN="numactl -N ${NUMA_NODES} -m ${NUMA_NODES} ${BENCH_BIN}"
fi


########################################################################################
# Run benchmark
########################################################################################

source "${CONFIG_ENV}"

for IMPL in ${IMPL_CANDIDATES}; do
  for THREAD_NUM in ${THREAD_CANDIDATES}; do
    for LOOP in `seq ${BENCH_REPEAT_COUNT}`; do
      echo -n "${IMPL},${THREAD_NUM},"
      ${BENCH_BIN} \
        --csv \
        --throughput=${MEASURE_THROUGHPUT} \
        --use-priority-queue=${USE_PRIORITY_QUEUE} \
        --${IMPL} \
        --num_exec ${OPERATION_COUNT} \
        --num_thread ${THREAD_NUM} \
        ${PMEM_DIR}
    done
  done
done
