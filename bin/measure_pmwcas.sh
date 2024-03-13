#!/bin/bash

set -u

########################################################################################
# Documents
########################################################################################

BENCH_BIN=""
CONFIG_ENV=""
PMEM_DIR=""
NUMA_NODES=""
MEASURE_THROUGHPUT="t"
readonly WORKSPACE_DIR=$(cd $(dirname ${BASH_SOURCE:-${0}})/.. && pwd)
readonly RANDOM_ID=$(cat /dev/urandom | base64 | tr -dc 'a-zA-Z0-9' | head -c 10)
readonly TMP_PATH="/tmp/pmwcas_benchmark-$(id -un)-${RANDOM_ID}"

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
  -l: Use latency as a criteria (default: false).
EOS
  exit 1
}

########################################################################################
# Parse options
########################################################################################

while getopts n:lht OPT
do
  case ${OPT} in
    n) NUMA_NODES=${OPTARG}
      ;;
    t) MEASURE_THROUGHPUT="t"
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
  for BLOCK_SIZE in ${BLOCK_SIZE_CANDIDATES}; do
    for SKEW_PARAMETER in ${SKEW_CANDIDATES}; do
      for TARGET_NUM in ${TARGET_CANDIDATES}; do
        if [ "${IMPL}" = "pcas" -a "${TARGET_NUM}" -ne "1" ]; then
          continue
        fi
        for THREAD_NUM in ${THREAD_CANDIDATES}; do
          for LOOP in `seq ${BENCH_REPEAT_COUNT}`; do
            TMP_OUTPUT="${TMP_PATH}-output-$(date +%Y%m%d-%H%m%S-%N).csv"
            while : ; do
              timeout "90s" \
                ${BENCH_BIN} \
                --${IMPL} \
                --csv \
                --throughput=${MEASURE_THROUGHPUT} \
                --num_exec ${OPERATION_COUNT} \
                --num_thread ${THREAD_NUM} \
                --skew_parameter ${SKEW_PARAMETER} \
                --arr-cap ${ARRAY_CAPACITY} \
                --block-size ${BLOCK_SIZE} \
                --timeout ${TIMEOUT} \
                ${PMEM_DIR} \
                ${TARGET_NUM} \
                >> "${TMP_OUTPUT}"
              if [ ${?} -eq 0 ]; then
                break
              fi
            done
            sed \
              "s/^/${IMPL},${BLOCK_SIZE},${TARGET_NUM},${SKEW_PARAMETER},${THREAD_NUM},/g" \
              "${TMP_OUTPUT}"
            rm -f "${TMP_OUTPUT}"
          done
        done
      done
    done
  done
done
