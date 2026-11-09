#pragma once

#include "../src/types.h"

#include <string>
#include <vector>

namespace benchmarking {

// Return struct for the benchmarking results
struct BenchmarkResult {
    std::size_t query_count;
    double seconds;
    double queries_per_second;
    double result_checksum;
};

// Reads testing_files/<filename>. Header lines: query count, then dimension.
std::vector<Point> read_queries(const std::string& filename);

// File parsing and index loading occur before timing. Only process_requests is
// timed; the returned points are consumed after timing to retain the computation.
BenchmarkResult run_benchmark(const std::string& filename, const std::string& implementation);

} // namespace benchmarking
