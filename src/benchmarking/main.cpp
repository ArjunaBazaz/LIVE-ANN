#include "benchmark.h"

#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void usage() {
    std::cout << "Usage: benchmark --filename <queries.txt> --implementation cpu_static_base\n Run from the repository root. Reads testing_files/<queries.txt>.\n";
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
            auto& value = flag == "--filename" ? filename : implementation;
            if (!value.empty()) throw std::invalid_argument("Duplicate flag: " + flag);
            value = argv[++i];
        }
        if (filename.empty() || implementation.empty()) {
            throw std::invalid_argument("--filename and --implementation are required.");
        }
        const auto result = benchmarking::run_benchmark(filename, implementation);
        std::cout << "Implementation: " << implementation << '\n'
                  << "Queries: " << result.query_count << '\n'
                  << std::fixed << std::setprecision(6)
                  << "Total time: " << result.seconds << " seconds\n"
                  << "Throughput: " << result.queries_per_second << " queries/second\n"
                  << "Result checksum: " << result.result_checksum << '\n';

    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        usage();
        return 1;
    }
}
