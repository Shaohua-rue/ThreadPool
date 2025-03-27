cmake_minimum_required(VERSION 3.0)

project(threadPool)

include_directories(
    include
)

add_library(threadPool
    src/threadPool.cpp
)
add_executable(main
    src/main.cpp
)
target_link_libraries(
    main
    threadPool
)