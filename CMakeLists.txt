cmake_minimum_required(VERSION 3.16)
project("liloo")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_SHARED_LIBRARY_PREFIX "")

find_package(Python REQUIRED COMPONENTS Interpreter Development)
message(STATUS "Find Python executable: ${Python_EXECUTABLE}")
message(STATUS "Find Python site-packages: ${Python_SITELIB}")
message(STATUS "Find Python liloo: ${Python_SITELIB}/pybind11")

list(APPEND CMAKE_PREFIX_PATH ${Python_SITELIB}/pybind11)
find_package(pybind11 REQUIRED)

# Required to use `SHARED` library instead of `MODULE` (by default)
# Because later library should be linked and worked opened with `dlopen`
pybind11_add_module(libliloo SHARED liloo/core.cpp)

target_include_directories(libliloo PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include> # for headers when building
    $<INSTALL_INTERFACE:include>                  # for client in install mode
)

set_target_properties(libliloo PROPERTIES PUBLIC_HEADER
    "include/liloo/core.h"
)

include(GNUInstallDirs)
install(TARGETS libliloo
    EXPORT libliloo
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/liloo
    LIBRARY DESTINATION lib
)
install(FILES cmake/libliloo-config.cmake DESTINATION lib/libliloo)
install(EXPORT libliloo DESTINATION lib/libliloo)
