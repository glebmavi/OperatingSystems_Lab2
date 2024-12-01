#ifndef IO_LATENCY_READ_BENCHMARK_H
#define IO_LATENCY_READ_BENCHMARK_H

#include <string>

namespace IOLatencyReadBenchmark {
    void run(const std::string& file_path, int iterations, bool verbose, bool use_cache);
}

#endif
