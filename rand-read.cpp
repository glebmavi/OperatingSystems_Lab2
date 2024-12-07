#include "BenchUtils/BenchmarkMain.h"
#include "Bench/RandReadBenchmark.h"
#include <iostream>

int main(const int argc, char* argv[]) {
    std::cout << "=== Starting " << argv[0] << " benchmark ===" << std::endl;

    auto benchmark_func = [](const BenchmarkConfig& config) -> int {
        RandReadBenchmark::run(config.read_path, config.iterations, config.verbose, config.use_cache);
        return 0;
    };

    if (!run_benchmark_main(argc, argv, true, benchmark_func)) {
        return 1;
    }

    return 0;
}