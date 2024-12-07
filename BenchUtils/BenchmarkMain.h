// BenchmarkMain.h
#pragma once

#include "BenchmarkConfig.h"
#include "BenchmarkUtils.cpp"
#include <functional>

bool run_benchmark_main(const int argc, char* argv[], const bool require_read_path, std::function<int(const BenchmarkConfig&)> benchmark_func) {
    BenchmarkConfig config;
    config.program_name = argv[0];

    if (!parse_arguments(argc, argv, config, require_read_path)) {
        return false;
    }

    return benchmark_func(config) == 0;
}
