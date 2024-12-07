#ifndef RANDREADBENCHMARK_H
#define RANDREADBENCHMARK_H

#include <string>

namespace RandReadBenchmark {
    void run(const std::string& file_path, int iterations, bool verbose, bool use_cache);
}

#endif //RANDREADBENCHMARK_H
