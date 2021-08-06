
conan config set general.cmake_generator=Ninja
IF NOT EXIST build (mkdir build)
cd build
conan install .. --build=missing -s build_type=Release
conan install .. --build=missing -s build_type=Debug
cd ..
pause