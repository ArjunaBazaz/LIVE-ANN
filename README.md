# LIVE-ANN

Live Ingestion Vector Engine for Approximate Nearest Neighbors.

## Overview

LIVE-ANN is an in-memory inverted file (IVF) for approximate nearest neighbor (ANN)
search. It handles accelerating IVF on multi-core CPUs and is optimized for static workloads.

## Centroid Loading Instructions

TODO

## Running Instructions

Run the following commands from the repository root in Bash/Linux or WSL.
Requirements: Python 3 and a C++17 compiler with OpenMP support.

1. Build the benchmark:

```bash
mkdir -p sift
g++ -std=c++17 -O3 -fopenmp -Isrc \
src/benchmarking/main.cpp src/benchmarking/benchmark.cpp \
src/ivf_static_cpu_base.cpp src/distance.cpp \
src/data_processing/loader.cpp -o sift/benchmark.exe
```

3. Generate a query file. Arguments are query count, dimension, and filename:

```bash
python3 test_generation/generate_queries.py *query count* *dimensions* *filename*
```

4. Run the benchmark and choose the OpenMP thread count:

```bash
OMP_NUM_THREADS=8 ./sift/benchmark.exe \
--filename queries_random_10000.txt --implementation cpu_static_base
```

Pass only the filename to `--filename`; it is resolved inside `testing_files/`.
The currently supported implementation is `cpu_static_base`. Use
`OMP_NUM_THREADS=1` for the single-thread comparison.

## Results

TODO

## Implementation details

TODO

## Filetree

```text
LIVE-ANN/
├── docs/
│   └── index.html
├── src/
│   ├── benchmarking/
│   │   ├── benchmark.cpp
│   │   ├── benchmark.h
│   │   └── main.cpp
│   ├── data_processing/
│   │   ├── loader.cpp
│   │   └── loader.h
│   ├── distance.cpp
│   ├── distance.h
│   ├── ivf_static_cpu_base.cpp
│   ├── ivf_static_cpu_base.h
│   └── types.h
├── test_generation/
│   └── generate_queries.py
├── .gitignore
├── LICENSE
└── README.md
```
