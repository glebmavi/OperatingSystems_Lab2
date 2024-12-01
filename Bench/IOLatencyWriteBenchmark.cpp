#include "IOLatencyWriteBenchmark.h"
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

namespace IOLatencyWriteBenchmark {

    constexpr size_t BLOCK_SIZE = 512; // Writing in blocks of 512 bytes
    constexpr size_t FILE_SIZE = 1024 * 1024; // File size of 1 MB

    void run(const int iterations, const bool verbose, const bool use_cache) {
        const std::vector buffer(BLOCK_SIZE, 'a'); // Buffer for writing
        std::vector<double> durations; // Time durations for each iteration

        for (int i = 0; i < iterations; ++i) {
            remove("testfile.dat"); // Delete the file if it already exists

            auto start = std::chrono::high_resolution_clock::now();

            if (use_cache) {
                // Using the lab2_cache API
                const int fd = lab2_open("testfile.dat", O_CREAT | O_RDWR | O_TRUNC, 0644);
                if (fd < 0) {
                    std::cerr << "Error opening file for IO benchmark!" << std::endl;
                    return;
                }

                // Writing data to the file in blocks
                for (size_t written = 0; written < FILE_SIZE; written += BLOCK_SIZE) {
                    ssize_t ret = lab2_write(fd, buffer.data(), BLOCK_SIZE);
                    if (ret != BLOCK_SIZE) {
                        std::cerr << "Error writing to file during IO benchmark!" << std::endl;
                        lab2_close(fd);
                        return;
                    }
                }
                lab2_fsync(fd);
                lab2_close(fd);
            } else {
                // Use standard file IO with 0_DIRECT
                const int fd = open("testfile.dat", O_CREAT | O_RDWR | O_TRUNC | O_DIRECT, 0644);
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
                std::memset(aligned_buffer, 'a', BLOCK_SIZE);

                // Write data to the file in blocks
                for (size_t written = 0; written < FILE_SIZE; written += BLOCK_SIZE) {
                    ssize_t ret = write(fd, aligned_buffer, BLOCK_SIZE);
                    if (ret != static_cast<ssize_t>(BLOCK_SIZE)) {
                        std::cerr << "Error writing to file during IO benchmark!" << std::endl;
                        free(aligned_buffer);
                        close(fd);
                        return;
                    }
                }
                fsync(fd);
                free(aligned_buffer);
                close(fd);
            }

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> duration = end - start;
            durations.push_back(duration.count());

            if (verbose) std::cout << "IO Iteration " << i + 1
                      << ": Write latency = " << duration.count() << " seconds"
                      << std::endl;
        }

        // Calculating overall statistics
        const double avg_duration = std::accumulate(durations.begin(), durations.end(), 0.0) / durations.size();
        const double min_duration = *std::ranges::min_element(durations);
        const double max_duration = *std::ranges::max_element(durations);

        std::cout << "\nOverall Statistics:\n";
        std::cout << "Average write latency: " << avg_duration << " seconds\n";
        std::cout << "Minimum write latency: " << min_duration << " seconds\n";
        std::cout << "Maximum write latency: " << max_duration << " seconds\n";
    }
}
