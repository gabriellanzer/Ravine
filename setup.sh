#!/bin/bash

conan config set general.cmake_generator=Ninja
[ ! -d "./build" ] && mkdir build
cd build
conan install .. --build=missing -s build_type=Release
conan install .. --build=missing -s build_type=Debug
cd ..
$SHELL