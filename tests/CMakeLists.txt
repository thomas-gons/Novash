cmake_minimum_required(VERSION 3.15)
project(Novash LANGUAGES C)

# --- Compilation setup ---
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enable all warnings
if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# --- Source files ---
file(GLOB_RECURSE NOVASH_SOURCES
    ../src/*.c
    ../src/**/*.c
    *.c
)

# --- Main executable ---
add_executable(nsh_tests ${NOVASH_SOURCES})
target_include_directories(nsh_tests PRIVATE ../src /usr/include/readline)
target_link_libraries(nsh_tests PRIVATE readline)

# --- Criterion setup for tests ---
find_package(criterion REQUIRED)

# --- Test executable ---
file(GLOB_RECURSE TEST_SOURCES
    tests/*.c
)

if (TEST_SOURCES)
    add_executable(tests_novash ${TEST_SOURCES})
    target_include_directories(tests_novash PRIVATE include)
    target_link_libraries(tests_novash PRIVATE Criterion::Criterion readline)
    enable_testing()
    add_test(NAME NovashTests COMMAND tests_novash)
endif()

# --- Install target (optional) ---
install(TARGETS nsh_tests DESTINATION bin)

# --- Print summary ---
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C Compiler: ${CMAKE_C_COMPILER}")
