cmake_minimum_required(VERSION 3.10)

project(Shell C)
set(CMAKE_C_STANDARD 23)

file(GLOB SOURCES "src/*.c")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-parameter")

add_executable(Shell ${SOURCES})
target_include_directories(Shell PRIVATE src)