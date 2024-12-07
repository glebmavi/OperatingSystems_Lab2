#pragma once

#include <string>

struct BenchmarkConfig {
    std::string program_name;
    std::string read_path; // Empty if not required
    int iterations;
    bool verbose = false;
    bool use_cache = false;
};
