cmake_minimum_required(VERSION 3.0)
project(threadPoolProject)

include_directories(
    include
)

# 创建动态库
add_library(threadPool SHARED
    src/task.cpp
    src/threadPool.cpp
)

# 创建可执行文件
add_executable(main
    src/main.cpp
)
target_link_directories(main PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib)
# 链接动态库
target_link_libraries(main threadPool)

# 安装动态库到lib目录
install(TARGETS threadPool
    LIBRARY DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

# 安装可执行文件到bin目录
install(TARGETS main
    RUNTIME DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/bin
)