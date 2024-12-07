#include "RandReadBenchmark.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <numeric>
#include <algorithm>
#include <random>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstring>
#include "../API/lab2_cache.h"

namespace RandReadBenchmark {

    constexpr size_t BLOCK_SIZE = 512; // Reading in blocks of 512 bytes

    void run(const std::string& file_path, const int iterations, const bool verbose, const bool use_cache) {
        std::vector<char> buffer(BLOCK_SIZE); // Buffer for reading
        std::vector<double> durations; // Time durations for each iteration
        durations.reserve(iterations);

        // Get file size for random offset generation
        struct stat st = {};
        if (stat(file_path.c_str(), &st) != 0) {
            std::cerr << "Failed to get file size!" << std::endl;
            return;
        }
        const off_t file_size = st.st_size;

        if (file_size == 0) {
            std::cerr << "File is empty. Cannot perform random reads." << std::endl;
            return;
        }

        // Prepare random offset generator with block alignment
        std::random_device rd;
        std::mt19937 gen(rd());
        const off_t max_block_index = file_size >= BLOCK_SIZE ? (file_size - BLOCK_SIZE) / BLOCK_SIZE : 0;
        std::uniform_int_distribution<off_t> block_dist(0, max_block_index);

        for (int i = 0; i < iterations; ++i) {
            const off_t random_offset = block_dist(gen) * BLOCK_SIZE;

            auto start = std::chrono::high_resolution_clock::now();

            if (use_cache) {
                // Using the lab2_cache API
                const int fd = lab2_open(file_path.c_str(), O_RDONLY, 0);
                if (fd < 0) {
                    std::cerr << "Error opening file for random read benchmark!" << std::endl;
                    return;
                }

                // Seek to random offset
                if (lab2_lseek(fd, random_offset, SEEK_SET) < 0) { //TODO: Does lseek really move the file pointer?
                    std::cerr << "Error seeking in file!" << std::endl;
                    lab2_close(fd);
                    return;
                }

                ssize_t bytes_read = lab2_read(fd, buffer.data(), BLOCK_SIZE);
                if (bytes_read < 0) {
                    std::cerr << "Error reading from file during random read benchmark!" << std::endl;
                    lab2_close(fd);
                    return;
                }

                lab2_close(fd);
            } else {
                // Use standard file IO with O_DIRECT
                const int fd = open(file_path.c_str(), O_RDONLY | O_DIRECT);
                if (fd < 0) {
                    std::cerr << "Error opening file for random read benchmark!" << std::endl;
                    return;
                }

                // Allocate an aligned buffer
                void* aligned_buffer = nullptr;
                if (posix_memalign(&aligned_buffer, 512, BLOCK_SIZE) != 0) {
                    std::cerr << "Error allocating aligned buffer!" << std::endl;
                    close(fd);
                    return;
                }

                // Seek to random offset
                if (lseek(fd, random_offset, SEEK_SET) < 0) {
                    std::cerr << "Error seeking in file: " << std::strerror(errno) << std::endl;
                    free(aligned_buffer);
                    close(fd);
                    return;
                }

                ssize_t bytes_read = read(fd, aligned_buffer, BLOCK_SIZE);
                if (bytes_read < 0) {
                    std::cerr << "Error reading from file during random read benchmark: " << std::strerror(errno) << std::endl;
                    free(aligned_buffer);
                    close(fd);
                    return;
                }

                free(aligned_buffer);
                close(fd);
            }

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> duration = end - start;
            durations.push_back(duration.count());

            if (verbose) {
                std::cout << "Random Read Iteration " << i + 1
                          << ": Latency = " << duration.count() << " seconds"
                          << std::endl;
            }
        }

        // Calculating overall statistics
        const double avg_duration = std::accumulate(durations.begin(), durations.end(), 0.0) / durations.size();
        const double min_duration = *std::ranges::min_element(durations);
        const double max_duration = *std::ranges::max_element(durations);

        std::cout << "\n=== Random Read Benchmark Results ===\n";
        std::cout << "Average read latency: " << avg_duration << " seconds\n";
        std::cout << "Minimum read latency: " << min_duration << " seconds\n";
        std::cout << "Maximum read latency: " << max_duration << " seconds\n";

        std::cout << "Cache hits: " << lab2_get_cache_hits() << std::endl;
        std::cout << "Cache misses: " << lab2_get_cache_misses() << std::endl;
        lab2_reset_cache_counters();
    }

}
