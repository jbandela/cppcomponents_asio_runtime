#! /bin/sh

g++ -std=c++11 ../../implementation/cppcomponents_asio_runtime_dll.cpp -I ../../../cppcomponents -I ../../implementation/external_dependencies  -fvisibility=hidden -Bstatic -lssl -lcrypto -lboost_regex  -fPIC -shared -pthread -o cppcomponents_asio_dll.so 
