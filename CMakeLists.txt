cmake_minimum_required(VERSION 3.0)
project(threadPo0lProject)

include_directories(
    include
)

add_executable(main
src/main.cpp
src/threadPool.cpp
)