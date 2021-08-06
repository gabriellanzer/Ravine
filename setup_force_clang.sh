#!/bin/bash
export CC=clang
export CXX=clang++
conan config set general.cmake_generator=Ninja
[ ! -d "./build" ] && mkdir build
cd build
conan install .. --build=missing -s build_type=Release -s cppstd=17
conan install .. --build=missing -s build_type=Debug -s cppstd=17
cd ..
$SHELL