# PMwCAS Benchmark

[![Ubuntu-22.04](https://github.com/dbgroup-nagoya-u/pmwcas-benchmark/actions/workflows/unit_tests.yaml/badge.svg?branch=main)](https://github.com/dbgroup-nagoya-u/pmwcas-benchmark/actions/workflows/unit_tests.yaml)

## Build

### Prerequisites

Note: `libnuma-dev` is required to build microsoft/pmwcas.

```bash
sudo apt update && sudo apt install -y build-essential cmake libgflags-dev libpmemobj-cpp-dev libnuma-dev
cd <path_to_your_workspace>
git clone --recursive git@github.com:dbgroup-nagoya-u/pmwcas-benchmark.git
cd pmwcas-benchmark
```

### Build Options

#### Parameters for Benchmarking

- `PMWCAS_BENCH_ELEMENT_NUM`: The number of elements in a target array (default: `1E6`).
- `PMWCAS_BENCH_TARGET_NUM`: The maximum number of target words of PMwCAS (default: `3`).
- `PMWCAS_BENCH_MAX_THREAD_NUM`: The maximum number of threads for benchmarking (default: `256`).

#### Parameters for Unit Testing

- `PMWCAS_BENCH_BUILD_TESTS`: build unit tests for this repository if `ON` (default: `OFF`).
- `DBGROUP_TEST_THREAD_NUM`: The number of worker threads for testing (default: `8`).
- `DBGROUP_TEST_TMP_PMEM_PATH`: The path to a durable storage (default: `/tmp`).

### Build and Run Unit Tests

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DPMWCAS_BENCH_BUILD_TESTS=ON ..
make -j
ctest -C Release
```

## Usage

The following command displays available CLI options:

```bash
./build/pmwcas_bench --helpshort
./build/queue_bench --helpshort
```

We prepare scripts in `bin` directory to measure performance with a variety of parameters. You can set parameters for benchmarking by `config/bench.env`.
