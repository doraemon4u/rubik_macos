cmake_minimum_required(VERSION 3.13)
project(min2phase)

set(CMAKE_CXX_STANDARD 14)

include_directories(include)

add_library(min2phase STATIC
        src/info.h
        src/CubieCube.h
        src/coords.h
        src/Search.h
        src/tests.h
        src/http.h
        src/info.cpp
        src/CubieCube.cpp
        src/coords.cpp
        src/Search.cpp
        src/tools.cpp
        src/min2phase.cpp
        src/http.cpp
        src/tests.cpp)

target_link_libraries(min2phase pthread)

add_executable(example
        examples/example.cpp)

add_executable(benchmark
        examples/benchmark.cpp)

add_executable(tests
        examples/tests.cpp)

add_executable(server-example
        examples/server-example.cpp)

target_link_libraries(example min2phase)
target_link_libraries(benchmark min2phase)
target_link_libraries(tests min2phase)
target_link_libraries(server-example min2phase)