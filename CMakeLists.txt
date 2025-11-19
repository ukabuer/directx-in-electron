cmake_minimum_required(VERSION 3.9)

project (my-renderer)

set (CMAKE_CXX_STANDARD 20)

add_executable(${PROJECT_NAME} native/main.cpp)
