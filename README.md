# MwCAS Benchmark

[![Ubuntu-20.04](https://github.com/dbgroup-nagoya-u/mwcas-benchmark/actions/workflows/unit_tests.yaml/badge.svg?branch=main)](https://github.com/dbgroup-nagoya-u/mwcas-benchmark/actions/workflows/unit_tests.yaml)

## Build

### Prerequisites

Note: `libnuma-dev` is required to build PMwCAS.

```bash
sudo apt update && sudo apt install -y build-essential cmake libgflags-dev libnuma-dev
cd <path_to_your_workspace>
git clone --recursive git@github.com:dbgroup-nagoya-u/mwcas-benchmark.git
cd mwcas-benchmark
```

### Build Options

#### Parameters for Benchmarking

- `MWCAS_BENCH_TARGET_NUM`: the number of target words of MwCAS (default: `2`).
- `MWCAS_BENCH_OVERRIDE_JEMALLOC`: override entire memory allocation with jemalloc if `ON` (default: `OFF`).
    - We assume that jemalloc is configured with the following command.

    ```bash
    ./configure --prefix=/usr/local --with-version=VERSION
    ```

#### Parameters for Unit Testing

- `MWCAS_BENCH_BUILD_TESTS`: build unit tests for this repository if `ON` (default: `OFF`).

### Build and Run Unit Tests

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DMWCAS_BENCH_BUILD_TESTS=ON ..
make -j
ctest -C Release
```

## Usage

The following command displays available CLI options:

```bash
./build/mwcas_bench --helpshort
```

We prepare scripts in `bin` directory to measure performance with a variety of parameters. You can set parameters for benchmarking by `config/bench.env`.

## Acknowledgments

This work is based on results obtained from project JPNP16007 commissioned by the New Energy and Industrial Technology Development Organization (NEDO). In addition, this work was supported partly by KAKENHI (16H01722 and 20K19804).
