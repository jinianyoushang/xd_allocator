cmake_minimum_required(VERSION 3.25)
project(xd_allocator)

set(CMAKE_CXX_STANDARD 17)

add_executable(xd_allocator src/main.cpp
        src/new_allocator.h
        src/pool_allocator.h
        src/Loki_allocator.h)

add_executable(testAllocator testAllocator.cpp
        testAllocator.cpp
)

add_executable(testSTLsample testSTLsample.cpp)
add_executable(testMemoryAllocator testMemoryAllocator.cpp)

include_directories(testMyAlloc)
add_executable(testMyAlloc src/testMyAlloc.cpp)
add_executable(testGetProcessMemoryInfo testGetProcessMemoryInfo.cpp)

