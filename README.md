# LIVE-ANN

Live Ingestion Vector Engine for Approximate Nearest Neighbors.

## Overview

LIVE-ANN is an in-memory inverted file (IVF) for approximate nearest neighbor (ANN)
search. It handles accelerating IVF on multi-core CPUs and is optimized for static workloads.

## Centroid Loading Instructions

The instructions below are steps to set up baseline centroids to replicate the experimental results from this project.

1. Centroids are loaded from SIFT1M, a public set of vectors. Download and extract using the following commands from a sift/ folder:

```bash
wget ftp://ftp.irisa.fr/local/texmex/corpus/sift.tar.gz
tar -xzf sift.tar.gz
```

2. Run centroid_generation/assign_centroids.cpp using the following commands to train k-means clustering and assign the remaining points to their nearest cluster:

```bash
g++ -std=c++17 -O3 -fopenmp \
  centroid_generation/train_kmeans.cpp \
  centroid_generation/assign_centroids.cpp \
  -o sift/assign_centroids.exe \
  -lfaiss -lopenblas -llapack

./sift/assign_centroids.exe \
  <training_file> <database_file> \
  <k> <output_prefix> <iterations> <seed> <batch_size>
```

3. Run pack_l2_index.cpp using the commands below. This produces sift/sift_l2.packed.bin,
which is then used by the programs below:

```bash
g++ -std=c++17 -O3 \
  centroid_generation/pack_l2_index.cpp \
  -o sift/pack_l2_index.exe

./sift/pack_l2_index.exe \
  sift/sift_l2 \
  sift/sift_base.fvecs \
  sift/sift_l2.packed.bin
```

## Running Instructions

Run the following commands from the repository root in Bash/Linux or WSL.
Requirements: Python 3 and a C++17 compiler with OpenMP support.

1. Build the benchmark:

```bash
mkdir -p sift
g++ -std=c++17 -O3 -fopenmp -Isrc \
src/benchmarking/main.cpp src/benchmarking/benchmark.cpp \
src/ivf_dynamic_cpu_coarse_streaming.cpp src/distance.cpp \
src/data_processing/loader.cpp -o sift/benchmark.exe
```

3. Generate a query file. Arguments are query count, dimension, and filename:

```bash
python3 test_generation/generate_queries.py *query count* *dimensions* *filename* *zipfian_skew*
```

4. Run the benchmark and choose the OpenMP thread count:

```bash
OMP_NUM_THREADS=8 ./sift/benchmark.exe \
--filename queries_random_10000.txt --implementation cpu_dynamic_coarse_streaming
```

Pass only the filename to `--filename`; it is resolved inside `testing_files/`.
The currently supported implementation is `cpu_dynamic_coarse_streaming`. Use
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
├── centroid_generation/
│   ├── assign_centroids.cpp
│   ├── pack_l2_index.cpp
│   ├── pack_l2_index.h
│   └── train_kmeans.cpp
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
│   ├── ivf_dynamic_cpu_coarse_streaming.cpp
│   ├── ivf_dynamic_cpu_coarse_streaming.h
│   └── types.h
├── test_generation/
│   └── generate_queries.py
├── .gitignore
├── LICENSE
└── README.md
```
