cmake_minimum_required(VERSION 3.16)

project("test_liloo_module")

set(CMAKE_CXX_STANDARD 17)

find_package(Python REQUIRED COMPONENTS Interpreter Development)

list(APPEND CMAKE_PREFIX_PATH ${Python_SITELIB}/pybind11)
find_package(pybind11 REQUIRED)

list(APPEND CMAKE_PREFIX_PATH ${Python_SITELIB}/liloo)
find_package(libliloo REQUIRED)

pybind11_add_module(libtest_module test_module.cpp)
target_link_libraries(libtest_module PRIVATE libliloo)
