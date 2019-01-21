cmake_minimum_required (VERSION 3.0)

project(Example2 C CXX)

find_package(Threads REQUIRED)

add_executable(Example2 async_stepped.cpp)

target_link_libraries(Example2 liberad ${CMAKE_THREAD_LIBS_INIT})
