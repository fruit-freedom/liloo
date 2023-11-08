project("Async inference engine")


set(CMAKE_SHARED_LIBRARY_PREFIX "")
add_library(asyncie SHARED
    # main2.cpp
    # eventfd.cpp
    liloo/liloo.cpp
)

include_directories(
    ${CMAKE_SOURCE_DIR}/third_party/pybind11/include
    /usr/include/python3.10/
)

link_libraries(libpython3.10)

