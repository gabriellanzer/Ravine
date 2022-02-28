cmake_minimum_required(VERSION 3.1.0)

# Project Setup
project(Ravine VERSION 0.1.1)

# Configure Conan
include(${CMAKE_SOURCE_DIR}/conan.cmake)

add_compile_definitions(EASTL_EASTDC_VSNPRINTF=0)
#add_definitions("-std=c++11")

conan_cmake_run(REQUIRES
				glfw/3.3.6
				eastl/3.18.00
				volk/1.3.204
				fmt/8.1.1
				glm/0.9.9.8
				spirv-cross/cci.20211113
				glslang/11.7.0
				openfbx/cci.20210426
                BASIC_SETUP NO_OUTPUT_DIRS
				IMPORTS "., license* -> ../licenses @ folder=True, ignore_case=True"
				IMPORTS "include, * -> ../include @ ignore_case=True"
				CMAKE_TARGETS BUILD missing)

# Configure Includes
include_directories(${CMAKE_SOURCE_DIR}/include)

# Configure Executable
include_directories(${CMAKE_SOURCE_DIR}/src)
file(GLOB_RECURSE SRC_FILES "${CMAKE_SOURCE_DIR}/src/*.c*" "${CMAKE_SOURCE_DIR}/src/*.h*")
add_executable(ravine ${SRC_FILES})

# Target for C++ STD Version 2017
target_compile_features(ravine PRIVATE cxx_std_17)
target_compile_options(ravine PRIVATE -I -pthread)
target_link_options(ravine PRIVATE -mwindows)

target_link_libraries(ravine CONAN_PKG::glfw)
target_link_libraries(ravine CONAN_PKG::eastl)
target_link_libraries(ravine CONAN_PKG::volk)
target_link_libraries(ravine CONAN_PKG::fmt)
target_link_libraries(ravine CONAN_PKG::glm)
target_link_libraries(ravine CONAN_PKG::spirv-cross)
target_link_libraries(ravine CONAN_PKG::glslang)
target_link_libraries(ravine CONAN_PKG::openfbx)

set_target_properties(
    ravine
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/${CMAKE_BUILD_TYPE}/"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/${CMAKE_BUILD_TYPE}/"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}/"
    OUTPUT_NAME "Ravine ${PROJECT_VERSION}"
)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})