# Utility Scripts for Experiments

## Usage

### Show Options

```bash
./bin/measure_pmwcas.sh -h
```

### Run Benchmark with Specified Configurations

```bash
./bin/measure_pmwcas.sh <bench_bin> <config> <pmem_dir> 1> results.csv 2> error.log
```

#### Example: Measure Throughput

```bash
./bin/measure_pmwcas.sh ./build/pmwcas_bench ./bin/bench.env /pmem_tmp 1> results.csv 2> error.log
```

#### Example: Measure Percentile Latency

```bash
./bin/measure_pmwcas.sh -l ./build/pmwcas_bench ./bin/bench.env /pmem_tmp 1> results.csv 2> error.log
```

## Configurations

### Parameters for Running Benchmark with Different Settings

- `THREAD_CANDIDATES`: The number of worker threads for executing PMwCAS.
- `TARGET_CANDIDATES`: The number of target words of PMwCAS.
- `SKEW_CANDIDATES`: A skew parameter in a Zipf distribution.
- `BLOCK_SIZE_CANDIDATES`: The size of memory blocks for storing target words.
- `IMPL_CANDIDATES`: A competitor for PMwCAS benchmark.

### Environment Settings

- `BENCH_REPEAT_COUNT`: The number of execution per setting.
- `OPERATION_COUNT`: The number of PMwCAS operations per worker.
- `ARRAY_CAPACITY`: The number of words in a PMwCAS target array.
- `TIMEOUT`: A timeout for each execution.
