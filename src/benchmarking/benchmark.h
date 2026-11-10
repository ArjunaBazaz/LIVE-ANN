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
    double result_checksum;
    std::size_t operation_count = 0;
    std::size_t insert_count = 0;
    std::size_t delete_count = 0;
    double operations_per_second = 0;
};

// Reads testing_files/<filename>. Header lines: query count, then dimension.
std::vector<Point> read_queries(const std::string& filename);

// Reads headerless rows: search|insert|delete followed by dimension floats.
std::vector<Operation> read_workload(const std::string& filename);

// File parsing and index loading occur before timing. Only process_requests is
// timed; the returned points are consumed after timing to retain the computation.
BenchmarkResult run_benchmark(const std::string& filename, const std::string& implementation);

} // namespace benchmarking
