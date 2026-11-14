#include "benchmark.h"

#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void usage() {
    std::cout << "Usage: benchmark --filename <file.txt> --implementation <cpu_static_base|cpu_dynamic_coarse_streaming|gpu_dynamic_coarse_streaming>\n"
        "Run from the repository root. Reads testing_files/<file.txt>.\n"
        "Static: numeric count/dimension headers and query rows. Dynamic: labeled operation rows without headers.\n";
}
} // namespace

// Handles command line flags
// Runs benchmark
int main(int argc, char* argv[]) {
    try {
        if (argc == 2 && std::string(argv[1]) == "--help") {
            usage();
            return 0;
        }
        std::string filename, implementation;
        for (int i = 1; i < argc; i++) {
            const std::string flag(argv[i]);
            if (flag != "--filename" && flag != "--implementation") {
                throw std::invalid_argument("Unknown flag: " + flag);
            }
            if (i + 1 >= argc || std::string(argv[i + 1]).rfind("--", 0) == 0) {
                throw std::invalid_argument("Missing value for " + flag);
            }
            auto& value = flag == "--filename" ? filename : implementation;
            if (!value.empty()) throw std::invalid_argument("Duplicate flag: " + flag);
            value = argv[++i];
        }
        if (filename.empty() || implementation.empty()) {
            throw std::invalid_argument("--filename and --implementation are required.");
        }
        const auto result = benchmarking::run_benchmark(filename, implementation);
        const bool mixed = implementation == "cpu_dynamic_coarse_streaming" ||
                           implementation == "gpu_dynamic_coarse_streaming";
        std::cout << "Implementation: " << implementation << '\n';
        if (mixed) std::cout << "Operations: " << result.operation_count << '\n';
        std::cout << "Queries: " << result.query_count << '\n';
        if (mixed) {
            std::cout << "Inserts: " << result.insert_count << '\n'
                      << "Deletes: " << result.delete_count << '\n';
        }
        std::cout << std::fixed << std::setprecision(6)
                  << "Total time: " << result.seconds << " seconds\n"
                  << "Throughput: " << result.operations_per_second
                  << (mixed ? " operations/second\n" : " queries/second\n")
                  << "Total sum squared distance: " << result.total_squared_distance << '\n';

    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        usage();
        return 1;
    }
}
