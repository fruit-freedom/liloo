cmake_minimum_required(VERSION 3.16)

project("Test module with liloo")

# set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-allow-shlib-undefined")
# set(CMAKE_LINK_WHAT_YOU_USE ON)
set(CMAKE_CXX_STANDARD 17)

add_library(test_module SHARED test_module.cpp)
# add_executable(libtest_module test_module.cpp)

include_directories(
    /home/sasha/liloo/third_party/pybind11/include
    /usr/include/python3.8/
)

target_link_directories(test_module PUBLIC
    /home/sasha/liloo/build
)

target_link_libraries(test_module PUBLIC
    # libpython3.10
    liloo
)


