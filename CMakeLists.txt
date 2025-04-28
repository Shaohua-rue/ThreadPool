cmake_minimum_required(VERSION 3.0)
project(threadpool)

include_directories(
    include
)
add_executable(main
src/main.cpp)
target_link_libraries(main pthread)