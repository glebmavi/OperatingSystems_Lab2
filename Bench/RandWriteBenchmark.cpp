#include "RandWriteBenchmark.h"
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

namespace RandWriteBenchmark {

    constexpr size_t BLOCK_SIZE = 512;
    constexpr size_t FILE_SIZE = 1024 * 1024; // File size of 1 MB
    constexpr auto DEFAULT_FILE_PATH = "testfile.dat";

    void run(const int iterations, const bool verbose, const bool use_cache) {
        const std::string file_path = DEFAULT_FILE_PATH;

        const std::vector buffer(BLOCK_SIZE, 'B');
        std::vector<double> durations;
        durations.reserve(iterations);

        // Get file size and ensure it's large enough
        struct stat st = {};
        if (stat(file_path.c_str(), &st) != 0) {
            std::cerr << "Failed to get file size for " << file_path << ": " << std::strerror(errno) << std::endl;
            return;
        }
        const size_t file_size = st.st_size;

        if (file_size < BLOCK_SIZE) {
            std::cerr << "File size must be at least " << BLOCK_SIZE << " bytes." << std::endl;
            return;
        }

        // Prepare random offset generator with block alignment
        std::random_device rd;
        std::mt19937 gen(rd());
        const off_t max_block_index = (file_size / BLOCK_SIZE) - 1;
        std::uniform_int_distribution<off_t> block_dist(0, max_block_index);

        for (int i = 0; i < iterations; ++i) {
            const off_t random_offset = block_dist(gen) * BLOCK_SIZE;

            auto start = std::chrono::high_resolution_clock::now();

            if (use_cache) {
                const int fd = lab2_open(file_path.c_str(), O_RDWR, 0);
                if (fd < 0) {
                    std::cerr << "Error opening file '" << file_path << "' for random write benchmark: "
                              << std::strerror(errno) << std::endl;
                    return;
                }

                if (lab2_lseek(fd, random_offset, SEEK_SET) < 0) {
                    std::cerr << "Error seeking in file '" << file_path << "': "
                              << std::strerror(errno) << std::endl;
                    lab2_close(fd);
                    return;
                }

                for (size_t written = 0; written < FILE_SIZE; written += BLOCK_SIZE) {
                    ssize_t bytes_written = lab2_write(fd, buffer.data(), BLOCK_SIZE);
                    if (bytes_written < 0) {
                        std::cerr << "Error writing to file '" << file_path
                                  << "' during random write benchmark: " << std::strerror(errno) << std::endl;
                        lab2_close(fd);
                        return;
                    }
                }

                lab2_fsync(fd);
                lab2_close(fd);
            } else {
                // Standard file IO with O_DIRECT for direct writes
                const int fd = open(file_path.c_str(), O_RDWR | O_DIRECT);
                if (fd < 0) {
                    std::cerr << "Error opening file '" << file_path << "' for random write benchmark: "
                              << std::strerror(errno) << std::endl;
                    return;
                }

                void* aligned_buffer = nullptr;
                if (posix_memalign(&aligned_buffer, 512, BLOCK_SIZE) != 0) {
                    std::cerr << "Error allocating aligned buffer!" << std::endl;
                    close(fd);
                    return;
                }

                std::memset(aligned_buffer, 'B', BLOCK_SIZE);

                if (lseek(fd, random_offset, SEEK_SET) < 0) {
                    std::cerr << "Error seeking in file '" << file_path << "': "
                              << std::strerror(errno) << std::endl;
                    free(aligned_buffer);
                    close(fd);
                    return;
                }

                for (size_t written = 0; written < FILE_SIZE; written += BLOCK_SIZE) {
                    ssize_t bytes_written = write(fd, aligned_buffer, BLOCK_SIZE);
                    if (bytes_written < 0) {
                        std::cerr << "Error writing to file '" << file_path
                                  << "' during random write benchmark: " << std::strerror(errno) << std::endl;
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

            if (verbose) {
                std::cout << "Random Write Iteration " << i + 1
                          << ": Latency = " << duration.count() << " seconds"
                          << std::endl;
            }
        }

        const double avg_duration = std::accumulate(durations.begin(), durations.end(), 0.0) / durations.size();
        const double min_duration = *std::ranges::min_element(durations);
        const double max_duration = *std::ranges::max_element(durations);

        std::cout << "=== Random Write Benchmark Results ===\n";
        std::cout << "Average write latency: " << avg_duration << " seconds\n";
        std::cout << "Minimum write latency: " << min_duration << " seconds\n";
        std::cout << "Maximum write latency: " << max_duration << " seconds\n";

        if (use_cache) {
            std::cout << "Cache hits: " << lab2_get_cache_hits() << std::endl;
            std::cout << "Cache misses: " << lab2_get_cache_misses() << std::endl;
            lab2_reset_cache_counters();
        }
    }

}
