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

#Flamegraph
#sudo perf record -F 4096 -g ./OS_Lab2_read testfile.dat 100 --use-cache
#sudo perf script | "/home/glebmavi/FlameGraph/stackcollapse-perf.pl" | "/home/glebmavi/FlameGraph/flamegraph.pl" > flamegraph.svg