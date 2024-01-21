cmake_minimum_required(VERSION 3.16)

project("Async CPP connector")
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_SHARED_LIBRARY_PREFIX "")

add_library(asyncie SHARED liloo/liloo.cpp)

add_library(libliloo SHARED liloo/core.cpp)

include_directories(
    ${CMAKE_SOURCE_DIR}/third_party/pybind11/include
    /usr/include/python3.10/
)

link_libraries(python3.10)
