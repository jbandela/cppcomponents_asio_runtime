#! /bin/sh

g++ -std=c++11 ../unit_test.cpp -I ../../../cppcomponents -I ../../../cppcomponents_concurrency/ -I ../external/googletest-read-only/ -I ../external/googletest-read-only/include/ ../external/googletest-read-only/src/gtest_main.cc ../external/googletest-read-only/src/gtest-all.cc -pthread -ldl -o unit_test
