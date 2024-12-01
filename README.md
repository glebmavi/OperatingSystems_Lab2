# Операционные системы. Лабораторная работа 2

## Вариант
- ОС: Linux
- Second-change

## Задание
Для оптимизации работы с блочными устройствами в ОС существует кэш страниц с данными, которыми мы производим операции чтения и записи на диск. Такой кэш позволяет избежать высоких задержек при повторном доступе к данным, так как операция будет выполнена с данными в RAM, а не на диске (вспомним пирамиду памяти).

В данной лабораторной работе необходимо реализовать блочный кэш в пространстве пользователя в виде динамической библиотеки (dll или so). Политика вытеснения страниц: Second-chance.

При выполнении работы необходимо реализовать простой API для работы с файлами, предоставляющий пользователю следующие возможности:
1. Открытие файла по заданному пути файла, доступного для чтения. Процедура возвращает некоторый хэндл на файл. Пример: `int lab2_open(const char* path);`
2. Закрытие файла по хэндлу. Пример: `int lab2_close(int fd);`
3. Чтение данных из файла. Пример: `ssize_t lab2_read(int fd, void buf[.count], size_t count);`
4. Запись данных в файл. Пример: `ssize_t lab2_write(int fd, const void buf[.count], size_t count);`
5. Перестановка позиции указателя на данные файла. Достаточно поддержать только абсолютные координаты. Пример: `off_t lab2_lseek(int fd, off_t offset, int whence);`
6. Синхронизация данных из кэша с диском. Пример: `int lab2_fsync(int fd);`

Операции с диском разработанного блочного кеша должны производиться в обход page cache используемой ОС.

В рамках проверки работоспособности разработанного блочного кэша необходимо адаптировать указанную преподавателем программу-загрузчик из ЛР 1, добавив использование кэша. Запустите программу и убедитесь, что она корректно работает. Сравните производительность до и после.

### Ограничения
- Программа (комплекс программ) должна быть реализован на языке C или C++.
- Если по выданному варианту задана политика вытеснения Optimal, то необходимо предоставить пользователю возможность подсказать page cache, когда будет совершен следующий доступ к данным. Это можно сделать либо добавив параметр в процедуры read и write (например, ssize_t lab2_read(int fd, void buf[.count], size_t count, access_hint_t hint)), либо добавив еще одну функцию в API (например, int lab2_advice(int fd, off_t offset, access_hint_t hint)). access_hint_t в данном случае – это абсолютное время или временной интервал, по которому разработанное API будет определять время последующего доступа к данным.
- Запрещено использовать высокоуровневые абстракции над системными вызовами. Необходимо использовать, в случае Unix, процедуры libc.

## Решение

### Структура проекта
```
.
├── API
│   ├── lab2_cache.cpp
│   └── lab2_cache.h
├── Bench
│   ├── IOLatencyReadBenchmark.cpp
│   ├── IOLatencyReadBenchmark.h
│   ├── IOLatencyWriteBenchmark.cpp
│   └── IOLatencyWriteBenchmark.h
├── CMakeLists.txt
├── io-lat-read.cpp
├── io-lat-write.cpp
├── README.md
└── run_bench.sh
```

[CMakeLists.txt](./CMakeLists.txt)
```cmake
cmake_minimum_required(VERSION 3.29)
project(OS_Lab2)

set(CMAKE_CXX_STANDARD 20)

# Dynamic library
add_library(lab2_cache SHARED API/lab2_cache.cpp)

# io-lat-write
add_executable(OS_Lab2_write io-lat-write.cpp
        Bench/IOLatencyWriteBenchmark.cpp
        Bench/IOLatencyWriteBenchmark.h)
target_link_libraries(OS_Lab2_write lab2_cache)
target_include_directories(OS_Lab2_write PRIVATE API)

# io-lat-read
add_executable(OS_Lab2_read io-lat-read.cpp
        Bench/IOLatencyReadBenchmark.cpp
        Bench/IOLatencyReadBenchmark.h)
target_link_libraries(OS_Lab2_read lab2_cache)
target_include_directories(OS_Lab2_read PRIVATE API)
```

### Программы-загрузчики
**io-lat-write**

Измерение задержки на запись накопителя с размерами блока Block Size
[IOLatencyWriteBenchmark.h](./Bench/IOLatencyWriteBenchmark.h)
```c++
#ifndef IO_LATENCY_WRITE_BENCHMARK_H
#define IO_LATENCY_WRITE_BENCHMARK_H

namespace IOLatencyWriteBenchmark {
    void run(int iterations, bool verbose, bool use_cache);
}

#endif
```

[IOLatencyWriteBenchmark.cpp](./Bench/IOLatencyWriteBenchmark.cpp)
```c++
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
        const std::vector<char> buffer(BLOCK_SIZE, 'a'); // Buffer for writing
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
```

[io-lat-write.cpp](./io-lat-write.cpp)
```c++
#include "Bench/IOLatencyWriteBenchmark.h"
#include <iostream>
#include <string>

void print_usage(const char* program_name) {
    std::cerr << "Usage: " << program_name << " <iterations> [-v] [--use-cache]" << std::endl;
}

int main(const int argc, char* argv[]) {

    std::cout << "=== Starting " << argv[0] <<" benchmark ===" << std::endl;

    if (argc < 2 || argc > 4) {
        print_usage(argv[0]);
        return 1;
    }

    int iterations;
    try {
        iterations = std::stoi(argv[1]);
        if (iterations <= 0) {
            throw std::invalid_argument("Iterations must be positive.");
        }
    } catch (const std::exception& e) {
        std::cerr << "Invalid iterations value: " << argv[1] << std::endl;
        return 1;
    }

    bool verbose = false;
    bool use_cache = false;
    for (int i = 2; i < argc; ++i) {
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

    IOLatencyWriteBenchmark::run(iterations, verbose, use_cache);

    return 0;
}
```

**io-lat-read**

Измерение задержки на чтение накопителя с размерами блока Block Size
[IOLatencyReadBenchmark.h](./Bench/IOLatencyReadBenchmark.h)
```c++
#ifndef IO_LATENCY_READ_BENCHMARK_H
#define IO_LATENCY_READ_BENCHMARK_H

#include <string>

namespace IOLatencyReadBenchmark {
    void run(const std::string& file_path, int iterations, bool verbose, bool use_cache);
}

#endif
```

[IOLatencyReadBenchmark.cpp](./Bench/IOLatencyReadBenchmark.cpp)
```c++
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
```

[io-lat-read.cpp](./io-lat-read.cpp)
```c++
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
```

### API блочного кэша
[lab2_cache.cpp](./API/lab2_cache.cpp)
```c++
#include "lab2_cache.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <map>
#include <list>
#include <vector>
#include <cstring>
#include <iostream>
#include <cerrno>
#include <sys/stat.h>

#define BLOCK_SIZE 4096     // Block size in bytes
#define MAX_CACHE_SIZE 64 // Max blocks in cache

struct CacheBlock {
    char* data;           // Pointer to block data
    bool is_dirty;        // Dirty flag (modified since read)
    bool was_accessed;    // Reference bit for Second-Chance
};

struct FileDescriptor {
    int fd;
    off_t offset;
};

typedef std::pair<int, off_t> CacheKey; // (file descriptor, block id corresponding to offset)

std::map<int, FileDescriptor> fd_table;     // Table for file descriptors
std::map<CacheKey, CacheBlock> cache_table;       // Table for cache blocks
std::list<CacheKey> cache_queue;            // Queue for Second-Chance

// Helper functions
FileDescriptor& get_file_descriptor(int fd);

/**
 * Frees a single cache block from memory.
 */
void free_cache_block() {
    while (true) {
        CacheKey key = cache_queue.front();
        cache_queue.pop_front();
        CacheBlock& block = cache_table[key];
        if (block.was_accessed) {
            // Has been referenced, so reset reference bit and move to back
            block.was_accessed = false;
            cache_queue.push_back(key);
        } else {
            if (block.is_dirty) { // Save to disk if dirty before removing
                const int fd = key.first;
                const off_t block_id = key.second;
                ssize_t ret = pwrite(fd, block.data, BLOCK_SIZE, block_id * BLOCK_SIZE);
                if (ret != BLOCK_SIZE) {
                    perror("pwrite");
                }
            }
            // Remove from cache
            free(block.data);
            cache_table.erase(key);
            break;
        }
    }
}

/**
 * Allocates an aligned buffer into memory.
 * @return Pointer to buffer
 */
char* allocate_aligned_buffer() {
    void* buf = nullptr;
    if (posix_memalign(&buf, BLOCK_SIZE, BLOCK_SIZE) != 0) {
        perror("posix_memalign");
        return nullptr;
    }
    return static_cast<char*>(buf);
}

/**
 * Opens a file with direct I/O.
 * @param path Path to file
 * @param flags File flags, same as in open()
 * @param mode File mode, same as in open()
 * @return Integer file descriptor
 */
int lab2_open(const char* path, int flags, mode_t mode) {
    const int fd = open(path, flags | O_DIRECT, mode);  // Read and write, and bypass OS cache
    if (fd < 0) {
        perror("open");
        return -1;
    }
    // For simplicity, use actual fd
    fd_table[fd] = {fd, 0};
    return fd;
}

/**
 * Closes a file.
 * @param fd File descriptor
 * @return 0 on success, -1 on failure
 */
int lab2_close(const int fd) {
    const auto iterator = fd_table.find(fd);
    if (iterator == fd_table.end()) {
        errno = EBADF;                           // Bad file descriptor
        return -1;
    }
    lab2_fsync(fd);                              // Sync data before closing
    const int result = close(iterator->second.fd);
    fd_table.erase(iterator);
    return result;
}

/**
 * Reads data from a file into a buffer.
 * @param fd File descriptor
 * @param buf Buffer to read into
 * @param count Number of bytes to read
 * @return Number of bytes read on success, -1 on failure
 */
ssize_t lab2_read(const int fd, void* buf, const size_t count) {
    auto& [found_fd, offset] = get_file_descriptor(fd);
    if (found_fd < 0 || offset < 0) {
        errno = EBADF;
        return -1;
    }
    size_t bytes_read = 0;
    const auto buffer = static_cast<char*>(buf);

    while (bytes_read < count) {
        off_t block_id = offset / BLOCK_SIZE;
        const size_t block_offset = offset % BLOCK_SIZE; // Offset within block
        const size_t bytes_to_read = std::min(BLOCK_SIZE - block_offset, count - bytes_read); // To read from block

        // Check if block is in cache
        CacheKey key = {found_fd, block_id};
        auto cache_iterator = cache_table.find(key);
        if (cache_iterator != cache_table.end()) {
            // HIT: Read from cache

            CacheBlock& found_block = cache_iterator->second;
            found_block.was_accessed = true;
            size_t available_bytes = BLOCK_SIZE - block_offset; // What's available in block
            const size_t bytes_from_block = std::min(bytes_to_read, available_bytes); // Compare what we need with what's available
            std::memcpy(buffer + bytes_read, found_block.data + block_offset, bytes_from_block);
            offset += bytes_from_block;
            bytes_read += bytes_from_block;
        } else {
            // MISS: Read block from disk

            if (cache_table.size() >= MAX_CACHE_SIZE) { // Cache is full, evict a block
                free_cache_block();
            }

            char* aligned_buf = allocate_aligned_buffer();
            ssize_t ret = pread(found_fd, aligned_buf, BLOCK_SIZE, block_id * BLOCK_SIZE);
            if (ret < 0) {
                // Failed to read
                perror("pread");
                free(aligned_buf);
                return -1;
            }
            if (ret == 0) {
                // EOF reached
                free(aligned_buf);
                break; // Exit the loop as there's no more data to read
            }
            // Partial read or full block read
            const auto valid_data_size = static_cast<size_t>(ret);
            // Fill the rest of the block with zeros if partial read
            if (ret < BLOCK_SIZE) {
                std::memset(aligned_buf + ret, 0, BLOCK_SIZE - ret);
            }
            // Add new block to cache
            const CacheBlock new_block = {aligned_buf, false, true};
            cache_table[key] = new_block;
            cache_queue.push_back(key);

            size_t available_bytes = valid_data_size - block_offset; // What's available in block
            if (available_bytes <= 0) {
                // No valid data available after the offset
                break;
            }
            const size_t bytes_from_block = std::min(bytes_to_read, available_bytes); // Compare what we need with what's available
            std::memcpy(buffer + bytes_read, aligned_buf + block_offset, bytes_from_block);
            offset += bytes_from_block;
            bytes_read += bytes_from_block;
        }
    }
    return bytes_read;
}

/**
 * Writes data from a buffer to a file.
 * @param fd File descriptor
 * @param buf Buffer to write from
 * @param count Number of bytes to write
 * @return Number of bytes written on success, -1 on failure
 */
ssize_t lab2_write(int fd, const void* buf, const size_t count) {
    auto& [found_fd, offset] = get_file_descriptor(fd);
    if (found_fd < 0 || offset < 0) {
        errno = EBADF;
        return -1;
    }
    size_t bytes_written = 0;
    const auto buffer = static_cast<const char*>(buf);

    while (bytes_written < count) {
        off_t block_id = offset / BLOCK_SIZE;
        const size_t block_offset = offset % BLOCK_SIZE;
        const size_t to_write = std::min(BLOCK_SIZE - block_offset, count - bytes_written);

        // Check if block is in cache
        CacheKey key = {found_fd, block_id};
        auto cache_it = cache_table.find(key);
        CacheBlock* block_ptr = nullptr;

        if (cache_it == cache_table.end()) { // MISS
            // If cache is full, evict a block
            if (cache_table.size() >= MAX_CACHE_SIZE) {
                free_cache_block();
            }
            // Read the block from disk to cache
            char* aligned_buf = allocate_aligned_buffer();
            ssize_t ret = pread(found_fd, aligned_buf, BLOCK_SIZE, block_id * BLOCK_SIZE);
            if (ret < 0) {
                // Read failed
                perror("pread");
                free(aligned_buf);
                return -1;
            } else if (ret == 0) {
                // If read fails (e.g., beyond EOF), fill with zeros
                std::memset(aligned_buf, 0, BLOCK_SIZE);
            } else if (ret < BLOCK_SIZE) {
                // Partial read, fill rest with zeros
                std::memset(aligned_buf + ret, 0, BLOCK_SIZE - ret);
            }
            // Add read block to cache
            CacheBlock& block = cache_table[key] = {aligned_buf, false, true};
            cache_queue.push_back(key);
            block_ptr = &block;
        } else { // HIT
            block_ptr = &cache_it->second;
        }
        // Write to cache
        std::memcpy(block_ptr->data + block_offset, buffer + bytes_written, to_write);
        block_ptr->is_dirty = true;
        block_ptr->was_accessed = true;

        offset += to_write;
        bytes_written += to_write;
    }
    return bytes_written;
}

/**
 * Sets file's offset. Only supports SEEK_SET (absolute positioning).
 * @param fd File descriptor
 * @param offset Absolute offset in bytes
 * @param whence Positioning mode (only SEEK_SET supported)
 * @return New file's offset on success, -1 on failure
 */
off_t lab2_lseek(const int fd, off_t offset, const int whence) {
    auto& [found_fd, file_offset] = get_file_descriptor(fd);
    if (found_fd < 0 || file_offset < 0) {
        errno = EBADF;
        return -1;
    }
    if (whence != SEEK_SET) {
        errno = EINVAL;
        return -1;
    }
    if (offset < 0) {
        errno = EINVAL;
        return -1;
    }
    file_offset = offset;
    return file_offset;
}

/**
 * Flushes dirty blocks to disk for a file descriptor.
 * @param fd_to_sync File descriptor to sync
 * @return 0 on success, -1 on failure
 */
int lab2_fsync(const int fd_to_sync) {
    const int found_fd = get_file_descriptor(fd_to_sync).fd;
    if (found_fd < 0) {
        errno = EBADF;
        return -1;
    }
    // Write back dirty blocks for this file descriptor
    for (auto& [key, block] : cache_table) {
        if (key.first == found_fd && block.is_dirty) {
            ssize_t ret = pwrite(found_fd, block.data, BLOCK_SIZE, key.second * BLOCK_SIZE);
            if (ret != BLOCK_SIZE) {
                perror("pwrite");
                return -1;
            }
            block.is_dirty = false;
        }
    }
    // Ensure all data and metadata are flushed to the physical storage
    return fsync(found_fd);
}

/**
 * Gets the file descriptor struct for a given file descriptor id.
 * @param fd File descriptor
 * @return FileDescriptor struct
 */
FileDescriptor& get_file_descriptor(const int fd) {
    const auto iterator = fd_table.find(fd);
    if (iterator == fd_table.end()) {
        static FileDescriptor invalid_fd = {-1, -1};
        return invalid_fd;
    }
    return iterator->second;
}
```

[lab2_cache.h](./API/lab2_cache.h)
```c++
#ifndef LAB2_CACHE_H
#define LAB2_CACHE_H

#include <cstddef>
#include <sys/types.h>

int lab2_open(const char* path, int flags, mode_t mode);

int lab2_close(int fd);

ssize_t lab2_read(int fd, void* buf, size_t count);

ssize_t lab2_write(int fd, const void* buf, size_t count);

off_t lab2_lseek(int fd, off_t offset, int whence);

int lab2_fsync(int fd_to_sync);

#endif
```


## Результаты

### Запуск
[run_bench.sh](./run_bench.sh)
```bash
#!/bin/bash

BUILD_DIR='cmake-build-debug'

cd $BUILD_DIR || exit

# Run the benchmark
./OS_Lab2_write 100
./OS_Lab2_write 100 --use-cache
./OS_Lab2_read testfile.dat 100
./OS_Lab2_read testfile.dat 100 --use-cache
```

### Write без кэша
Overall Statistics:
Average write latency: 0.0817999 seconds
Minimum write latency: 0.0788963 seconds
Maximum write latency: 0.0884632 seconds

### Write с кэшем
Overall Statistics:
Average write latency: 0.00737696 seconds
Minimum write latency: 0.00683776 seconds
Maximum write latency: 0.00983191 seconds

### Read без кэша
Overall Statistics:
Average read latency: 0.0282485 seconds
Minimum read latency: 0.0268514 seconds
Maximum read latency: 0.0321585 seconds

### Read с кэшем
Overall Statistics:
Average read latency: 0.00668384 seconds
Minimum read latency: 0.00609573 seconds
Maximum read latency: 0.00970559 seconds

### Сравнение производительности


### Выводы