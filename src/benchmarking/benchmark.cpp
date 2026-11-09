#include "benchmark.h"

#include "../src/data_processing/loader.h"
#include "../src/ivf_static_cpu_base.h"

#include <charconv>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace benchmarking {
namespace {
using ProcessRequests = std::vector<Point> (*)(const std::vector<Point>&, const Points&, const Metadata&);

// Handles the selection of what implementation to use.
// Options:
//  cpu_static_base - static IVF on multicore CPU using point based parallelism.
ProcessRequests select_implementation(const std::string& implementation) {
    if (implementation == "cpu_static_base") {
        return ivf_static_cpu_base::process_requests;
    }
    throw std::invalid_argument("Unknown implementation: " + implementation + ". Supported: cpu_static_base");
}

// Reads the header rows from the query file and loads the individual file
// Use for loading query count and number of dimensions
std::size_t read_header(std::istream& input, const char* name) {
    std::string line, token, extra;
    if (!std::getline(input, line)) {
        throw std::runtime_error(std::string("Missing header: ") + name);
    }
    std::istringstream row(line);
    if (!(row >> token) || (row >> extra)) {
        throw std::runtime_error(std::string("Expected one integer for ") + name);
    }
    std::size_t value = 0;
    const auto result = std::from_chars(token.data(), token.data() + token.size(), value);
    if (result.ec != std::errc{} || result.ptr != token.data() + token.size()) {
        throw std::runtime_error(std::string("Invalid integer for ") + name);
    }
    return value;
}
} // namespace

// Reads the queries after the header.
std::vector<Point> read_queries(const std::string& filename) {
    const std::filesystem::path name(filename);
    if (name.empty() || name.has_parent_path() || name == "." || name == "..") {
        throw std::invalid_argument("Pass a filename inside testing_files, without a directory.");
    }
    const auto path = std::filesystem::path("testing_files") / name;
    std::ifstream input(path);
    if (!input) throw std::runtime_error("Cannot open query file: " + path.string());
    const auto count = read_header(input, "query count");
    const auto vector_dimension = read_header(input, "dimension");
    if (vector_dimension != dimension) {
        throw std::runtime_error("Query dimension must be " + std::to_string(dimension) + ", got " + std::to_string(vector_dimension));
    }

    std::vector<Point> queries;
    std::string line;
    for (std::size_t i = 0; i < count; i++) {
        if (!std::getline(input, line)) {
            throw std::runtime_error("Missing query " + std::to_string(i + 1));
        }
        std::istringstream row(line);
        Point query;
        for (float& value : query) {
            if (!(row >> value) || !std::isfinite(value)) {
                throw std::runtime_error("Query " + std::to_string(i + 1) + " must contain " + std::to_string(dimension) + " finite floats.");
            }
        }

        queries.push_back(query);
    }

    if (input.bad()) throw std::runtime_error("Failed reading query file.");
    return queries;
}

// Runs the benchmark, reports the completion time and the throughput in terms of queries / second
BenchmarkResult run_benchmark(const std::string& filename, const std::string& implementation) {
    const auto process_requests = select_implementation(implementation);
    const auto queries = read_queries(filename);
    const auto [points, metadata] = sift::load_l2_index("sift/sift_l2.packed.bin");
    std::cout << "SIFT clusters loaded.\n";
    std::cout << "Test file loaded: " << filename << '\n';

    std::cout << "Starting benchmark." << std::endl;
    const auto start = std::chrono::steady_clock::now();
    const auto results = process_requests(queries, *points, metadata);
    const auto finish = std::chrono::steady_clock::now();

    const double seconds = std::chrono::duration<double>(finish - start).count();
    double checksum = 0;
    for (const auto& result : results) {
        for (float coordinate : result) checksum += coordinate;
    }
    const double throughput = seconds > 0 ? static_cast<double>(queries.size()) / seconds : 0;
    return {queries.size(), seconds, throughput, checksum};
}
} // namespace benchmarking
