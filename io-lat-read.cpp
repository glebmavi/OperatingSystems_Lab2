#include "Bench/IOLatencyReadBenchmark.h"
#include <iostream>
#include <string>

void print_usage(const char* program_name) {
    std::cerr << "Usage: " << program_name << " <read_path> <iterations> [-v] [--use-cache]" << std::endl;
}

int main(const int argc, char* argv[]) {

    std::cout << "=== Starting " << argv[0] <<" benchmark ===" << std::endl;

    if (argc < 3 || argc > 5) {
        print_usage(argv[0]);
        return 1;
    }

    std::string file_path = argv[1];
    int iterations;
    try {
        iterations = std::stoi(argv[2]);
        if (iterations <= 0) {
            throw std::invalid_argument("Iterations must be positive.");
        }
    } catch (const std::exception& e) {
        std::cerr << "Invalid iterations value: " << argv[1] << std::endl;
        return 1;
    }

    bool verbose = false;
    bool use_cache = false;
    for (int i = 3; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-v") {
            verbose = true;
        } else if (arg == "--use-cache") {
            use_cache = true;
        } else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            print_usage(argv[0]);
            return 1;
        }
    }

    IOLatencyReadBenchmark::run(file_path, iterations, verbose, use_cache);

    return 0;
}
