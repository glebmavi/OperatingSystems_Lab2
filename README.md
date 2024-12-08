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
│   ├── IOLatencyWriteBenchmark.h
│   ├── RandReadBenchmark.cpp
│   ├── RandReadBenchmark.h
│   ├── RandWriteBenchmark.cpp
│   └── RandWriteBenchmark.h
├── BenchUtils
│   ├── BenchmarkConfig.h
│   ├── BenchmarkMain.h
│   └── BenchmarkUtils.cpp
├── CMakeLists.txt
├── io-lat-read.cpp
├── io-lat-write.cpp
├── rand-read.cpp
├── rand-write.cpp
├── README.md
├── run_bench.sh
└── run_rand_bench.sh
```

[CMakeLists.txt](./CMakeLists.txt)
```cmake
cmake_minimum_required(VERSION 3.29)
project(OS_Lab2)

set(CMAKE_CXX_STANDARD 20)

#set(CMAKE_CXX_FLAGS_DEBUG "-O3 -g -march=native -mtune=native -flto -funroll-loops -fomit-frame-pointer")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

# Dynamic library
add_library(lab2_cache SHARED API/lab2_cache.cpp)

# io-lat-write
add_executable(OS_Lab2_write io-lat-write.cpp
        Bench/IOLatencyWriteBenchmark.cpp
        Bench/IOLatencyWriteBenchmark.h
        BenchUtils/BenchmarkMain.h)
target_link_libraries(OS_Lab2_write lab2_cache)
target_include_directories(OS_Lab2_write PRIVATE API)

# io-lat-read
add_executable(OS_Lab2_read io-lat-read.cpp
        Bench/IOLatencyReadBenchmark.cpp
        Bench/IOLatencyReadBenchmark.h
        BenchUtils/BenchmarkMain.h)
target_link_libraries(OS_Lab2_read lab2_cache)
target_include_directories(OS_Lab2_read PRIVATE API)

# rand-read
add_executable(OS_Lab2_rand_read rand-read.cpp
        Bench/RandReadBenchmark.cpp
        Bench/RandReadBenchmark.h
        BenchUtils/BenchmarkMain.h)
target_link_libraries(OS_Lab2_rand_read lab2_cache)
target_include_directories(OS_Lab2_rand_read PRIVATE API)

# rand-write
add_executable(OS_Lab2_rand_write rand-write.cpp
        Bench/RandWriteBenchmark.cpp
        Bench/RandWriteBenchmark.h
        BenchUtils/BenchmarkMain.h)
target_link_libraries(OS_Lab2_rand_write lab2_cache)
target_include_directories(OS_Lab2_rand_write PRIVATE API)
```

### Программы-загрузчики
**io-lat-write**

Измерение задержки на запись накопителя с размерами блока Block Size. Последовательная запись.

[IOLatencyWriteBenchmark.cpp](./Bench/IOLatencyWriteBenchmark.cpp)

Запуск с: [io-lat-write.cpp](./io-lat-write.cpp)

**io-lat-read**

Измерение задержки на чтение накопителя с размерами блока Block Size. Последовательное чтение.

[IOLatencyReadBenchmark.cpp](./Bench/IOLatencyReadBenchmark.cpp)

Запуск с: [io-lat-read.cpp](./io-lat-read.cpp)

**rand-write**

Измерение задержки на запись накопителя с размерами блока Block Size. Случайная запись.

[RandWriteBenchmark.cpp](./Bench/RandWriteBenchmark.cpp)

Запуск с: [rand-write.cpp](./rand-write.cpp)

**rand-read**

Измерение задержки на чтение накопителя с размерами блока Block Size. Случайное чтение.

[RandReadBenchmark.cpp](./Bench/RandReadBenchmark.cpp)

Запуск с: [rand-read.cpp](./rand-read.cpp)

### API блочного кэша

[lab2_cache.cpp](./API/lab2_cache.cpp)

Доступные функции:
```c++
int lab2_open(const char* path, int flags, mode_t mode);
int lab2_close(int fd);
ssize_t lab2_read(int fd, void* buf, size_t count);
ssize_t lab2_write(int fd, const void* buf, size_t count);
off_t lab2_lseek(int fd, off_t offset, int whence);
int lab2_fsync(int fd_to_sync);

// Stats
unsigned long lab2_get_cache_hits();
unsigned long lab2_get_cache_misses();
void lab2_reset_cache_counters();
```

Общие структуры и константы:
```c++
#define BLOCK_SIZE 4096     // Block size in bytes
#define MAX_CACHE_SIZE 32 // Max blocks in cache
// 4096 * 32 = 128KB

// Global counters for cache hits and misses
static unsigned long cache_hits = 0;
static unsigned long cache_misses = 0;

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
```


## Результаты

### Запуск
Для последовательного чтения и записи:
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


# With strace to know the amount of system calls to disk
strace -c ./OS_Lab2_write 100
strace -c ./OS_Lab2_write 100 --use-cache
strace -c ./OS_Lab2_read testfile.dat 100
strace -c ./OS_Lab2_read testfile.dat 100 --use-cache
```

Для случайного чтения и записи:
[run_rand_bench.sh](./run_rand_bench.sh)
```bash
#!/bin/bash

BUILD_DIR='cmake-build-debug'

cd $BUILD_DIR || exit

./OS_Lab2_rand_write 100
./OS_Lab2_rand_write 100 --use-cache
./OS_Lab2_rand_read testfile.dat 100
./OS_Lab2_rand_read testfile.dat 100 --use-cache

# With strace to know the amount of system calls to disk
strace -c ./OS_Lab2_rand_write 100
strace -c ./OS_Lab2_rand_write 100 --use-cache
strace -c ./OS_Lab2_rand_read testfile.dat 100
strace -c ./OS_Lab2_rand_read testfile.dat 100 --use-cache
```

### Write без кэша (Последовательная запись)
```
Overall Statistics:
Average write latency: 0.0822461 seconds
Minimum write latency: 0.0786764 seconds
Maximum write latency: 0.0933617 seconds
```

```
% time     seconds  usecs/call     calls    errors syscall
------ ----------- ----------- --------- --------- ----------------
99.58    1.246773           6    204805           write
0.19    0.002435          24       100           unlink
0.10    0.001298          12       100           fsync
0.10    0.001225          11       111         5 openat
0.03    0.000349           3       106           close
0.00    0.000000           0         5           read
0.00    0.000000           0         7           fstat
0.00    0.000000           0        26           mmap
0.00    0.000000           0         7           mprotect
0.00    0.000000           0         1           munmap
0.00    0.000000           0         3           brk
0.00    0.000000           0         2           pread64
0.00    0.000000           0         1         1 access
0.00    0.000000           0         1           execve
0.00    0.000000           0         1           arch_prctl
0.00    0.000000           0         1           futex
0.00    0.000000           0         1           set_tid_address
0.00    0.000000           0         2         2 newfstatat
0.00    0.000000           0         1           set_robust_list
0.00    0.000000           0         1           prlimit64
0.00    0.000000           0         1           getrandom
0.00    0.000000           0         1           rseq
------ ----------- ----------- --------- --------- ----------------
100.00    1.252080           6    205284         8 total
```

### Write с кэшем (Последовательная запись)
```
Overall Statistics:
Average write latency: 0.00752635 seconds
Minimum write latency: 0.00663992 seconds
Maximum write latency: 0.00988521 seconds
Cache hits: 179200
Cache misses: 25600
```

```
% time     seconds  usecs/call     calls    errors syscall
------ ----------- ----------- --------- --------- ----------------
 79.06    0.126326           4     25600           pwrite64
 18.59    0.029700           1     25602           pread64
  0.94    0.001499          14       100           unlink
  0.87    0.001393           6       200           fsync
  0.42    0.000679           6       111         5 openat
  0.11    0.000175           1       106           close
  0.01    0.000010           1         7           write
  0.00    0.000002           0         5           brk
  0.00    0.000000           0         5           read
  0.00    0.000000           0         7           fstat
  0.00    0.000000           0        26           mmap
  0.00    0.000000           0         7           mprotect
  0.00    0.000000           0         1           munmap
  0.00    0.000000           0         1         1 access
  0.00    0.000000           0         1           execve
  0.00    0.000000           0         1           arch_prctl
  0.00    0.000000           0         1           futex
  0.00    0.000000           0         1           set_tid_address
  0.00    0.000000           0         2         2 newfstatat
  0.00    0.000000           0         1           set_robust_list
  0.00    0.000000           0         1           prlimit64
  0.00    0.000000           0         1           getrandom
  0.00    0.000000           0         1           rseq
------ ----------- ----------- --------- --------- ----------------
100.00    0.159784           3     51788         8 total
```

### Read без кэша (Последовательное чтение)
```
Overall Statistics:
Average read latency: 0.0300317 seconds
Minimum read latency: 0.0288578 seconds
Maximum read latency: 0.0344697 seconds
```

```
% time     seconds  usecs/call     calls    errors syscall
------ ----------- ----------- --------- --------- ----------------
 99.90    0.759692           3    204905           read
  0.07    0.000514           4       111         5 openat
  0.03    0.000246           2       106           close
  0.00    0.000010           2         5           write
  0.00    0.000000           0         7           fstat
  0.00    0.000000           0        26           mmap
  0.00    0.000000           0         7           mprotect
  0.00    0.000000           0         1           munmap
  0.00    0.000000           0         3           brk
  0.00    0.000000           0         2           pread64
  0.00    0.000000           0         1         1 access
  0.00    0.000000           0         1           execve
  0.00    0.000000           0         1           arch_prctl
  0.00    0.000000           0         1           futex
  0.00    0.000000           0         1           set_tid_address
  0.00    0.000000           0         2         2 newfstatat
  0.00    0.000000           0         1           set_robust_list
  0.00    0.000000           0         1           prlimit64
  0.00    0.000000           0         1           getrandom
  0.00    0.000000           0         1           rseq
------ ----------- ----------- --------- --------- ----------------
100.00    0.760462           3    205184         8 total
```

### Read с кэшем (Последовательное чтение)
```
Overall Statistics:
Average read latency: 0.009879 seconds
Minimum read latency: 0.00938243 seconds
Maximum read latency: 0.0126887 seconds
Cache hits: 179200
Cache misses: 25700
```

```
% time     seconds  usecs/call     calls    errors syscall
------ ----------- ----------- --------- --------- ----------------
 98.65    0.059175           2     25702           pread64
  0.40    0.000241           2       111         5 openat
  0.34    0.000201           2       100           fsync
  0.28    0.000169           6        26           mmap
  0.20    0.000118           1       106           close
  0.04    0.000021           3         7           fstat
  0.03    0.000015           3         5           read
  0.02    0.000014           2         5           brk
  0.02    0.000011           1         7           mprotect
  0.01    0.000005           5         1           munmap
  0.01    0.000005           2         2         2 newfstatat
  0.01    0.000004           0         7           write
  0.01    0.000004           4         1         1 access
  0.00    0.000002           2         1           futex
  0.00    0.000001           1         1           prlimit64
  0.00    0.000001           1         1           getrandom
  0.00    0.000000           0         1           execve
  0.00    0.000000           0         1           arch_prctl
  0.00    0.000000           0         1           set_tid_address
  0.00    0.000000           0         1           set_robust_list
  0.00    0.000000           0         1           rseq
------ ----------- ----------- --------- --------- ----------------
100.00    0.059987           2     26088         8 total
```

### Write без кэша (Случайная запись)
```
Average write latency: 0.0306145 seconds
Minimum write latency: 0.0288675 seconds
Maximum write latency: 0.0560978 seconds
```

```
% time     seconds  usecs/call     calls    errors syscall
------ ----------- ----------- --------- --------- ----------------
 99.70    0.902319           4    204805           write
  0.15    0.001375          13       100           fsync
  0.06    0.000558           5       111         5 openat
  0.04    0.000336           3       106           close
  0.02    0.000164           6        26           mmap
  0.02    0.000163           1       100           lseek
  0.00    0.000043           6         7           mprotect
  0.00    0.000025           3         7           fstat
  0.00    0.000014           2         5           read
  0.00    0.000014          14         1           munmap
  0.00    0.000008           2         3         2 newfstatat
  0.00    0.000006           2         3           brk
  0.00    0.000004           2         2           pread64
  0.00    0.000003           3         1           arch_prctl
  0.00    0.000003           3         1           set_robust_list
  0.00    0.000003           3         1           getrandom
  0.00    0.000002           2         1           futex
  0.00    0.000002           2         1           set_tid_address
  0.00    0.000002           2         1           prlimit64
  0.00    0.000002           2         1           rseq
  0.00    0.000000           0         1         1 access
  0.00    0.000000           0         1           execve
------ ----------- ----------- --------- --------- ----------------
100.00    0.905046           4    205285         8 total
```

### Write с кэшем (Случайная запись)
```
Average write latency: 0.0156261 seconds
Minimum write latency: 0.00934271 seconds
Maximum write latency: 0.021561 seconds
Cache hits: 179206
Cache misses: 25594
```

```
Average write latency: 0.0192045 seconds
Minimum write latency: 0.0130736 seconds
Maximum write latency: 0.0303452 seconds
Cache hits: 179140
Cache misses: 25660
% time     seconds  usecs/call     calls    errors syscall
------ ----------- ----------- --------- --------- ----------------
 51.35    0.083012           3     25689           pwrite64
 47.27    0.076421           2     25662           pread64
  0.88    0.001421           7       200           fsync
  0.24    0.000396           3       111         5 openat
  0.12    0.000202           1       106           close
  0.07    0.000107           4        26           mmap
  0.02    0.000027           3         7           mprotect
  0.01    0.000020           2         7           write
  0.01    0.000011           2         5           read
  0.01    0.000011           1         7           fstat
  0.00    0.000008           1         5           brk
  0.00    0.000005           5         1           munmap
  0.00    0.000002           2         1           arch_prctl
  0.00    0.000002           2         1           set_tid_address
  0.00    0.000002           0         3         2 newfstatat
  0.00    0.000002           2         1           set_robust_list
  0.00    0.000002           2         1           getrandom
  0.00    0.000002           2         1           rseq
  0.00    0.000001           1         1           futex
  0.00    0.000001           1         1           prlimit64
  0.00    0.000000           0         1         1 access
  0.00    0.000000           0         1           execve
------ ----------- ----------- --------- --------- ----------------
100.00    0.161655           3     51838         8 total
```

### Read без кэша (Случайное чтение)
```
Average read latency: 0.0232364 seconds
Minimum read latency: 0.00130098 seconds
Maximum read latency: 0.0345497 seconds
```

```
% time     seconds  usecs/call     calls    errors syscall
------ ----------- ----------- --------- --------- ----------------
 99.84    0.708648           3    204805           read
  0.05    0.000357           3       111         5 openat
  0.03    0.000187           1       106           close
  0.02    0.000172           6        26           mmap
  0.02    0.000165         165         1           execve
  0.02    0.000127           1       100           lseek
  0.01    0.000039           5         7           mprotect
  0.00    0.000019           2         7           fstat
  0.00    0.000014          14         1           munmap
  0.00    0.000011           2         5           write
  0.00    0.000009           3         3           brk
  0.00    0.000005           2         2           pread64
  0.00    0.000005           1         3         2 newfstatat
  0.00    0.000004           4         1         1 access
  0.00    0.000003           3         1           arch_prctl
  0.00    0.000003           3         1           prlimit64
  0.00    0.000003           3         1           getrandom
  0.00    0.000002           2         1           futex
  0.00    0.000002           2         1           set_tid_address
  0.00    0.000002           2         1           set_robust_list
  0.00    0.000002           2         1           rseq
------ ----------- ----------- --------- --------- ----------------
100.00    0.709779           3    205185         8 total
```

### Read с кэшем (Случайное чтение)
```
Average read latency: 0.00582698 seconds
Minimum read latency: 0.00114094 seconds
Maximum read latency: 0.00987425 seconds
Cache hits: 143473
Cache misses: 61327
```

```
Average read latency: 0.00947416 seconds
Minimum read latency: 0.00708904 seconds
Maximum read latency: 0.0184305 seconds
Cache hits: 163794
Cache misses: 41006
% time     seconds  usecs/call     calls    errors syscall
------ ----------- ----------- --------- --------- ----------------
 98.50    0.075136           1     41008           pread64
  0.40    0.000303           3       100           fsync
  0.38    0.000289           2       111         5 openat
  0.27    0.000204         204         1           execve
  0.19    0.000146           1       106           close
  0.18    0.000137           5        26           mmap
  0.02    0.000017           2         7           fstat
  0.02    0.000015           3         5           read
  0.01    0.000011           2         5           brk
  0.01    0.000008           2         3         2 newfstatat
  0.01    0.000005           5         1         1 access
  0.01    0.000004           0         7           write
  0.00    0.000002           2         1           getrandom
  0.00    0.000000           0         7           mprotect
  0.00    0.000000           0         1           munmap
  0.00    0.000000           0         1           arch_prctl
  0.00    0.000000           0         1           futex
  0.00    0.000000           0         1           set_tid_address
  0.00    0.000000           0         1           set_robust_list
  0.00    0.000000           0         1           prlimit64
  0.00    0.000000           0         1           rseq
------ ----------- ----------- --------- --------- ----------------
100.00    0.076277           1     41395         8 total
```

### Сравнение производительности
Запущены программы-загрузчики для чтения и записи данных в файл размером 1 МБ (1,048,576 байт) с использованием блоков размером 512 байт.
Тесты проводились как с использованием блочного кэша, так и без него, для оценки влияния кэширования на производительность и количество системных вызовов.

Размеры блоков и кэша

- Размер блока в бенчмарках: 512 байт
- Размер блока в кэше: 4096 байт
- Количество блоков в файле:
  - При размере блока 512 байт: 1,048,576 / 512 = 2048 блоков 
  - При размере блока 4096 байт: 1,048,576 / 4096 = 256 блоков 
- Максимальный размер кэша: 1024 блока (1024 * 4096 байт = 4,194,304 байт или 4 МБ)

**Запись без кэша**

- Среднее время записи: 0.0822461 секунд
- Количество системных вызовов `write`: 204 805

Ожидалось, что количество записей будет 1 048 576 / 512 = 2048 вызовов `write`. При 100 итерациях: 204 800, что близко к реальному значению.

**Запись с кэшем**

- Среднее время записи: 0.0075996 секунд
- Количество системных вызовов `pwrite64`: 25 600
- Количество системных вызовов `pread64`: 25 602

Ожидалось, что количество записей будет 1 048 576 / 4096 = 256 вызовов `pwrite64`. При 100 итерациях: 25 600.
Время записи с кэшем уменьшилось в 10 раз, что логично учитывая меньшее количество обращений к диску.


**Чтение без кэша**

- Среднее время чтения: 0.0300317 секунд
- Количество системных вызовов `read`: 204 905

Аналогично ожидается 2048 вызовов `read` * 100 итераций = 204 800.

**Чтение с кэшем**

- Среднее время чтения: 0.00879 секунд
- Количество системных вызовов `pread64`: 25 702

Ожидаемое количество вызовов `pread64` = 256 * 100 = 25 600.
Время чтения с кэшем уменьшилось в 3.5 раза.

### Выводы
В ходе выполнения проекта был разработан блочный кэш в пространстве пользователя с политикой вытеснения Second-Chance. Проведенные тесты показали значительное улучшение производительности при использовании кэша как для операций чтения, так и для операций записи.