#! /bin/sh
# In this we assume that libssl.a and libcrypto.a and libboost_regex.a are in the current directory and are built with -fPIC. for openssl you can use ./config -fPIC and for boost you can use ./b2 cxxflags='-fPIC'

g++ -std=c++11 ../../implementation/cppcomponents_asio_runtime_dll.cpp -I ../../../cppcomponents -I ../../implementation/external_dependencies  -fvisibility=hidden -Bstatic libssl.a libcrypto.a libboost_regex.a  -fPIC -shared -pthread -o cppcomponents_asio_dll.so 
