#include "benchmark.h"

#include "../data_processing/loader.h"
#include "../ivf_dynamic_cpu_coarse_streaming.h"
#ifdef LIVE_ANN_ENABLE_CUDA
#include "../ivf_dynamic_gpu_coarse_streaming.h"
#endif

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace benchmarking {
namespace {

// Reads open the file for the benchmark run to test.
std::ifstream open_test_file(const std::string& filename) {
    const std::filesystem::path name(filename);
    if (name.empty() || name.has_parent_path() || name == "." || name == "..") {
        throw std::invalid_argument("Pass a filename inside testing_files, without a directory.");
    }
    const auto path = std::filesystem::path("testing_files") / name;
    std::ifstream input(path);
    return input;
}

} // namespace

// Reads the workload to test on from the testing_files folder.
std::vector<Operation> read_workload(const std::string& filename) {
    auto input = open_test_file(filename);
    std::vector<Operation> requests;
    std::string line;
    std::size_t line_number = 0;
    while (std::getline(input, line)) {
        line_number++;
        std::istringstream row(line);
        std::string label;
        if (!(row >> label)) continue;
        Operation request;
        if (label == "search") request.type = RequestType::Query;
        else if (label == "insert") request.type = RequestType::Insert;
        else if (label == "delete") request.type = RequestType::Delete;
        std::string extra;
        requests.push_back(request);
    }
    return requests;
}

// Runs the benchmark and reports completion time and operations per second.
BenchmarkResult run_benchmark(const std::string& filename, const std::string& implementation) {
    const auto requests = read_workload(filename);
    auto metadata = sift::load_l2_coarse_index("sift/sift_l2.packed.bin");
#ifdef LIVE_ANN_ENABLE_CUDA
    std::unique_ptr<Metadata_GPU> gpu_metadata;
    if (implementation == "gpu_dynamic_coarse_streaming") {
        gpu_metadata = std::make_unique<Metadata_GPU>(*metadata);
    }
#endif
    std::cout << "SIFT clusters loaded.\n";
    std::cout << "Test file loaded: " << filename << '\n';

    BenchmarkResult result{};
    result.operation_count = requests.size();
    for (const auto& request : requests) {
        switch (request.type) {
            case RequestType::Query: ++result.query_count; break;
            case RequestType::Insert: ++result.insert_count; break;
            case RequestType::Delete: ++result.delete_count; break;
        }
    }
    std::cout << "Starting benchmark." << std::endl;
    const auto start = std::chrono::steady_clock::now();
    const double total_squared_distance =
#ifdef LIVE_ANN_ENABLE_CUDA
        gpu_metadata
        ? ivf_dynamic_gpu_coarse_streaming::process_requests(requests, *gpu_metadata)
        :
#endif
        ivf_dynamic_cpu_coarse_streaming::process_requests(requests, *metadata);
    const auto finish = std::chrono::steady_clock::now();
    result.seconds = std::chrono::duration<double>(finish - start).count();
    result.total_squared_distance = total_squared_distance;
    if (result.seconds > 0) {
        result.queries_per_second = static_cast<double>(result.query_count) / result.seconds;
        result.operations_per_second = static_cast<double>(result.operation_count) / result.seconds;
    }
    return result;
}
} // namespace benchmarking
