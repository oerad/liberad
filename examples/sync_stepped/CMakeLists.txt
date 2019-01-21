cmake_minimum_required (VERSION 3.0)

project(Example3 C CXX)

find_package(Threads REQUIRED)

add_executable(Example3 sync_stepped.cpp)

target_link_libraries(Example3 liberad ${CMAKE_THREAD_LIBS_INIT})
