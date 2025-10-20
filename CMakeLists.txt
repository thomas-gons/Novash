cmake_minimum_required(VERSION 3.10)

project(nsh C)
set(CMAKE_C_STANDARD 23)

file(GLOB SOURCES "src/*.c")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g -Wall -Wextra -Wno-unused-parameter")

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE src /usr/include/readline)
target_link_libraries(${PROJECT_NAME} PRIVATE readline)
