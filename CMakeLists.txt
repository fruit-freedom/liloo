cmake_minimum_required(VERSION 3.16)
project("liloo")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_SHARED_LIBRARY_PREFIX "")

find_package(Python REQUIRED COMPONENTS Interpreter Development)
message(STATUS "Find Python executable: ${Python_EXECUTABLE}")
message(STATUS "Find Python site-packages: ${Python_SITELIB}")
message(STATUS "Find Python liloo: ${Python_SITELIB}/liloo")
message(STATUS "Find Python pybind11: ${Python_SITELIB}/pybind11")

list(APPEND CMAKE_PREFIX_PATH ${Python_SITELIB}/pybind11)
find_package(pybind11 REQUIRED)

pybind11_add_module(libliloo liloo/core.cpp)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
)

# add_library(asyncie SHARED liloo/liloo.cpp)
# include_directories(
#     ${Python_INCLUDE_DIRS}
#     ${pybind11_INCLUDE_DIR}
# )
# link_libraries(${Python_LIBRARIES})
