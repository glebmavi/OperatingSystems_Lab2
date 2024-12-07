#include "BenchmarkConfig.h"
#include <iostream>
#include <string>
#include <stdexcept>

void print_usage(const BenchmarkConfig& config, bool require_read_path) {
    std::cerr << "Usage: " << config.program_name;
    if (require_read_path) {
        std::cerr << " <read_path>";
    }
    std::cerr << " <iterations> [-v] [--use-cache]" << std::endl;
}

bool parse_arguments(int argc, char* argv[], BenchmarkConfig& config, bool require_read_path) {
    if (require_read_path) {
        if (argc < 3 || argc > 5) {
            print_usage(config, require_read_path);
            return false;
        }
        config.read_path = argv[1];
        try {
            config.iterations = std::stoi(argv[2]);
            if (config.iterations <= 0) throw std::invalid_argument("Iterations must be positive.");
        } catch (const std::exception& e) {
            std::cerr << "Invalid iterations value: " << argv[2] << std::endl;
            return false;
        }
        for (int i = 3; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "-v") {
                config.verbose = true;
            } else if (arg == "--use-cache") {
                config.use_cache = true;
            } else {
                std::cerr << "Unknown argument: " << arg << std::endl;
                print_usage(config, require_read_path);
                return false;
            }
        }
    } else {
        if (argc < 2 || argc > 4) {
            print_usage(config, require_read_path);
            return false;
        }
        try {
            config.iterations = std::stoi(argv[1]);
            if (config.iterations <= 0) throw std::invalid_argument("Iterations must be positive.");
        } catch (const std::exception& e) {
            std::cerr << "Invalid iterations value: " << argv[1] << std::endl;
            return false;
        }
        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "-v") {
                config.verbose = true;
            } else if (arg == "--use-cache") {
                config.use_cache = true;
            } else {
                std::cerr << "Unknown argument: " << arg << std::endl;
                print_usage(config, require_read_path);
                return false;
            }
        }
    }
    return true;
}
