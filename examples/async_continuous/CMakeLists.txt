cmake_minimum_required (VERSION 3.0)

project(Example1 C CXX)

find_package(Threads REQUIRED)

add_executable(Example1 async_cont.cpp)

target_link_libraries(Example1 liberad ${CMAKE_THREAD_LIBS_INIT})
