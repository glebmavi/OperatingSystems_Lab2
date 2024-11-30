#include "IOLatencyWriteBenchmark.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <numeric>
#include <algorithm>
#include "../API/lab2_cache.h"

namespace IOLatencyWriteBenchmark {

    constexpr size_t BLOCK_SIZE = 512; // Размер блока данных
    constexpr size_t FILE_SIZE = 1024 * 1024; // Размер файла в байтах

    void run(int iterations, bool verbose, bool use_cache) {
        std::vector<char> buffer(BLOCK_SIZE, 'a'); // Буфер для записи данных
        std::vector<double> durations; // Сохранение времени каждой итерации

        for (int i = 0; i < iterations; ++i) {
            // Удаляем существующий файл
            remove("testfile.dat");

            auto start = std::chrono::high_resolution_clock::now();

            if (use_cache) {
                // Используем API блочного кэша
                int fd = lab2_open("testfile.dat");
                if (fd < 0) {
                    std::cerr << "Ошибка при открытии файла для IO бенчмарка!" << std::endl;
                    return;
                }

                // Запись данных в файл по блокам
                for (size_t written = 0; written < FILE_SIZE; written += BLOCK_SIZE) {
                    ssize_t ret = lab2_write(fd, buffer.data(), BLOCK_SIZE);
                    if (ret != BLOCK_SIZE) {
                        std::cerr << "Ошибка при записи в файл во время IO бенчмарка!" << std::endl;
                        lab2_close(fd);
                        return;
                    }
                }

                lab2_fsync(fd);
                lab2_close(fd);
            } else {
                // Используем стандартные файловые операции
                std::ofstream file("testfile.dat", std::ios::binary | std::ios::trunc);

                if (!file) {
                    std::cerr << "Ошибка при открытии файла для IO бенчмарка!" << std::endl;
                    return;
                }

                // Запись данных в файл по блокам
                for (size_t written = 0; written < FILE_SIZE; written += BLOCK_SIZE) {
                    file.write(buffer.data(), BLOCK_SIZE);
                    if (!file) {
                        std::cerr << "Ошибка при записи в файл во время IO бенчмарка!" << std::endl;
                        file.close();
                        return;
                    }
                }

                file.flush();
                file.close();
            }

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> duration = end - start;
            durations.push_back(duration.count()); // Сохраняем время

            if (verbose) {
                std::cout << "IO Итерация " << i + 1
                          << ": Время записи = " << duration.count() << " секунд"
                          << std::endl;
            }
        }

        // Вычисление общей статистики
        double avg_duration = std::accumulate(durations.begin(), durations.end(), 0.0) / durations.size();
        double min_duration = *std::min_element(durations.begin(), durations.end());
        double max_duration = *std::max_element(durations.begin(), durations.end());

        // Вывод статистики
        std::cout << "\nОбщая статистика:\n";
        std::cout << "Среднее время записи: " << avg_duration << " секунд\n";
        std::cout << "Минимальное время записи: " << min_duration << " секунд\n";
        std::cout << "Максимальное время записи: " << max_duration << " секунд\n";
    }

}
