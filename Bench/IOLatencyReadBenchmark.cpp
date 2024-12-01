#include "IOLatencyReadBenchmark.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <numeric>
#include <algorithm>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include "../API/lab2_cache.h"

namespace IOLatencyReadBenchmark {

    constexpr size_t BLOCK_SIZE = 512; // Reading in blocks of 512 bytes

    void run(const std::string& file_path, const int iterations, const bool verbose, const bool use_cache) {
        std::vector<char> buffer(BLOCK_SIZE); // Buffer for reading
        std::vector<double> durations; // Time durations for each iteration

        for (int i = 0; i < iterations; ++i) {
            auto start = std::chrono::high_resolution_clock::now();

            if (use_cache) {
                // Using the lab2_cache API
                const int fd = lab2_open(file_path.c_str(), O_RDONLY, 0);
                if (fd < 0) {
                    std::cerr << "Error opening file for IO benchmark!" << std::endl;
                    return;
                }

                ssize_t bytes_read = 0;
                do {
                    bytes_read = lab2_read(fd, buffer.data(), BLOCK_SIZE);
                    if (bytes_read < 0) {
                        std::cerr << "Error reading from file during IO benchmark!" << std::endl;
                        lab2_close(fd);
                        return;
                    }
                } while (bytes_read > 0);

                lab2_close(fd);
            } else {
                // Use standard file IO with O_DIRECT
                const int fd = open(file_path.c_str(), O_RDONLY | O_DIRECT);
                if (fd < 0) {
                    std::cerr << "Error opening file for IO benchmark!" << std::endl;
                    return;
                }

                // Allocate an aligned buffer
                void* aligned_buffer = nullptr;
                if (posix_memalign(&aligned_buffer, 512, BLOCK_SIZE) != 0) {
                    std::cerr << "Error allocating aligned buffer!" << std::endl;
                    close(fd);
                    return;
                }

                ssize_t bytes_read = 0;
                do {
                    bytes_read = read(fd, aligned_buffer, BLOCK_SIZE);
                    if (bytes_read < 0) {
                        std::cerr << "Error reading from file during IO benchmark!" << std::endl;
                        free(aligned_buffer);
                        close(fd);
                        return;
                    }
                } while (bytes_read > 0);

                free(aligned_buffer);
                close(fd);
            }

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> duration = end - start;
            durations.push_back(duration.count());

            if (verbose) {
                std::cout << "IO Iteration " << i + 1
                          << ": Read latency = " << duration.count() << " seconds"
                          << std::endl;
            }
        }

        // Calculating overall statistics
        const double avg_duration = std::accumulate(durations.begin(), durations.end(), 0.0) / durations.size();
        const double min_duration = *std::ranges::min_element(durations);
        const double max_duration = *std::ranges::max_element(durations);

        std::cout << "\nOverall Statistics:\n";
        std::cout << "Average read latency: " << avg_duration << " seconds\n";
        std::cout << "Minimum read latency: " << min_duration << " seconds\n";
        std::cout << "Maximum read latency: " << max_duration << " seconds\n";
    }
}
