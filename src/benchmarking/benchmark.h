#pragma once

#include "../types.h"

#include <string>
#include <vector>

namespace benchmarking {

// Return struct for the benchmarking results
struct BenchmarkResult {
    std::size_t query_count;
    double seconds;
    double queries_per_second;
    double total_squared_distance;
    std::size_t operation_count = 0;
    std::size_t insert_count = 0;
    std::size_t delete_count = 0;
    double operations_per_second = 0;
};

// Reads headerless rows: search|insert|delete followed by dimension floats.
std::vector<Operation> read_workload(const std::string& filename);

// File parsing and index loading occur before timing. Only process_requests is
// timed, including summing the nearest squared distances for all queries.
BenchmarkResult run_benchmark(const std::string& filename, const std::string& implementation);

} // namespace benchmarking
