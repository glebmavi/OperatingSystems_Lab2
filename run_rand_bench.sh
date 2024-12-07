#!/bin/bash

BUILD_DIR='cmake-build-debug'

cd $BUILD_DIR || exit

./OS_Lab2_rand_read testfile.dat 100
./OS_Lab2_rand_read testfile.dat 100 --use-cache
./OS_Lab2_rand_write 100
./OS_Lab2_rand_write 100 --use-cache