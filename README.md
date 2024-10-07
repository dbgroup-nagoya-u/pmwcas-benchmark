# PMwCAS Benchmark

[![Ubuntu 24.04](https://github.com/dbgroup-nagoya-u/pmwcas-benchmark/actions/workflows/ubuntu_24.yaml/badge.svg)](https://github.com/dbgroup-nagoya-u/pmwcas-benchmark/actions/workflows/ubuntu_24.yaml) [![Ubuntu 22.04](https://github.com/dbgroup-nagoya-u/pmwcas-benchmark/actions/workflows/ubuntu_22.yaml/badge.svg)](https://github.com/dbgroup-nagoya-u/pmwcas-benchmark/actions/workflows/ubuntu_22.yaml)

- [Build](#build)
    - [Prerequisites](#prerequisites)
    - [Build Options](#build-options)
    - [Build and Run Unit Tests](#build-and-run-unit-tests)
- [Usage](#usage)

## Build

### Prerequisites

Note: `libnuma-dev` is required to build microsoft/pmwcas.

```bash
sudo apt update && sudo apt install -y \
  build-essential \
  cmake \
  libgflags-dev \
  libpmemobj-dev \
  libnuma-dev
cd <path_to_your_workspace>
git clone https://github.com/dbgroup-nagoya-u/pmwcas-benchmark.git
cd pmwcas-benchmark
```

### Build Options

#### Parameters for Benchmarking

- `PMWCAS_BENCH_MAX_TARGET_NUM`: The maximum number of target words of PMwCAS (default: `8`).
- `PMEM_ATOMIC_USE_DIRTY_FLAG`: Use dirty flags in PMwCAS to indicate words that are not persistent (please refer to [pmem-atomic](https://github.com/dbgroup-nagoya-u/pmem-atomic)).
- `DBGROUP_MAX_THREAD_NUM`: The maximum number of worker threads (please refer to [cpp-utility](https://github.com/dbgroup-nagoya-u/cpp-utility)).

#### Parameters for Unit Testing

- `PMWCAS_BENCH_BUILD_TESTS`: build unit tests for this repository if `ON` (default: `OFF`).
- `DBGROUP_TEST_THREAD_NUM`: The number of worker threads for testing (default: `0`).
- `DBGROUP_TEST_TMP_PMEM_PATH`: A path to durable storage (default: `""`).
    - If a path is not set, the PMEM-related tests will be skipped.

### Build and Run Unit Tests

```bash
mkdir build && cd build
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DPMWCAS_BENCH_BUILD_TESTS=ON \
  -DDBGROUP_TEST_TMP_PMEM_PATH=/pmem_tmp
cmake --build . --parallel --config Release
ctest -C Release
```

## Usage

The following command displays available CLI options:

```bash
./build/pmwcas_bench --helpshort
```

The benchmark program requires a path to persistent memory and the number of target words.

```bash
./build/pmwcas_bench --<competitor> <path_to_pmem_dir> <target_word_num>
```

For example, the following command performs P3wCAS benchmark with our PMwCAS implementation.

```bash
./build/pmwcas_bench --pmwcas /pmem_tmp/ 3
```

We prepare scripts in `bin` directory to measure performance with a variety of parameters.
