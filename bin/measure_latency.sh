#!/bin/bash
set -ue

########################################################################################
# Documents
########################################################################################

NUMA_NODES=""
WORKSPACE_DIR=$(cd $(dirname ${BASH_SOURCE:-${0}})/.. && pwd)

usage() {
  cat 1>&2 << EOS
Usage:
  ${BASH_SOURCE:-${0}} <bench_bin> <config> 1> results.csv 2> error.log
Description:
  Run MwCAS benchmark to measure latency. All the benchmark results are output in CSV
  format.
Arguments:
  <bench_bin>: Path to the binary file for benchmarking.
  <config>: Path to the configuration file for benchmarking.
Options:
  -N: Only execute benchmark on the CPUs of nodes. See "man numactl" for details.
  -h: Show this messsage and exit.
EOS
  exit 1
}

########################################################################################
# Parse options
########################################################################################

while getopts n:h OPT
do
  case ${OPT} in
    N) NUMA_NODES=${OPTARG}
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

if [ ${#} != 2 ]; then
  usage
fi

BENCH_BIN=${1}
CONFIG_ENV=${2}
if [ -n "${NUMA_NODES}" ]; then
  BENCH_BIN="numactl -N ${NUMA_NODES} -m ${NUMA_NODES} ${BENCH_BIN}"
fi

########################################################################################
# Run benchmark
########################################################################################

source "${CONFIG_ENV}"

for IMPL in ${IMPL_CANDIDATES}; do
  for SKEW_PARAMETER in ${SKEW_CANDIDATES}; do
    for THREAD_NUM in ${THREAD_CANDIDATES}; do
      if [ ${IMPL} == 0 ]; then
        IMPL_ARGS="--mwcas=t --pmwcas=f --aopt=f --single=f"
      elif [ ${IMPL} == 1 ]; then
        IMPL_ARGS="--mwcas=f --pmwcas=t --aopt=f --single=f"
      elif [ ${IMPL} == 2 ]; then
        IMPL_ARGS="--mwcas=f --pmwcas=f --aopt=t --single=f"
      elif [ ${IMPL} == 3 ]; then
        IMPL_ARGS="--mwcas=f --pmwcas=f --aopt=f --single=t"
      else
        continue
      fi
      for LOOP in `seq ${BENCH_REPEAT_COUNT}`; do
        echo -n "${IMPL},${SKEW_PARAMETER},${THREAD_NUM},"
        ${BENCH_BIN} \
          --csv --throughput=f ${IMPL_ARGS} --num-init-thread ${INIT_THREAD_NUM} \
          --num-field ${TARGET_FIELD_NUM} --num_exec ${OPERATION_COUNT} \
          --num_thread ${THREAD_NUM} --skew_parameter ${SKEW_PARAMETER}
      done
    done
  done
done
