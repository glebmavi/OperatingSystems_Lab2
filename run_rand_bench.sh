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