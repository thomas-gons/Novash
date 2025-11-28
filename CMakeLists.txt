cmake_minimum_required(VERSION 3.16)
project(novash
    VERSION 0.1.0
    LANGUAGES C
)

# -----------------------
# Build type
# -----------------------
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()
set(allowed_build_types Debug Release RelWithDebInfo MinSizeRel)
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${allowed_build_types})

# -----------------------
# Configurable options
# -----------------------
option(ENABLE_SANITIZERS "Enable ASAN/UBSAN in Debug builds" OFF)
option(WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(ENABLE_TESTS "Enable building tests using Criterion" OFF)

# -----------------------
# Log level configuration
# -----------------------
set(LOG_LEVEL "NONE" CACHE STRING "Set log level: NONE, ERR, WARN, INFO, DEBUG")
set_property(CACHE LOG_LEVEL PROPERTY STRINGS NONE ERR WARN INFO DEBUG)

# Map string values to integers
if(LOG_LEVEL STREQUAL "NONE")
    set(LOG_LEVEL_INT 0)
elseif(LOG_LEVEL STREQUAL "ERR")
    set(LOG_LEVEL_INT 1)
elseif(LOG_LEVEL STREQUAL "WARN")
    set(LOG_LEVEL_INT 2)
elseif(LOG_LEVEL STREQUAL "INFO")
    set(LOG_LEVEL_INT 3)
elseif(LOG_LEVEL STREQUAL "DEBUG")
    set(LOG_LEVEL_INT 4)
else()
    message(WARNING "Unknown LOG_LEVEL='${LOG_LEVEL}', defaulting to WARN")
    set(LOG_LEVEL_INT 2)
endif()

message(STATUS "Log level set to ${LOG_LEVEL} (${LOG_LEVEL_INT})")

# -----------------------
# C standard
# -----------------------
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# -----------------------
# Default compiler warnings
# -----------------------
if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(DEFAULT_WARNINGS
        -Wall
        -Wextra
        -Wpedantic
        -Wformat=2
        -Wcast-align
        -Wconversion
        -Wno-unused-parameter
        -Wmissing-prototypes
        -Wstrict-prototypes
        -Wold-style-declaration
    )
elseif (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
    set(DEFAULT_WARNINGS /W4)
endif()

function(novash_target_enable_warnings target)
    if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE ${DEFAULT_WARNINGS})
        if (WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    elseif (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${target} PRIVATE ${DEFAULT_WARNINGS})
    endif()
endfunction()

# -----------------------
# Project sources
# -----------------------
set(NOVASH_SOURCES
    # lexer
    src/lexer/lexer.c
    
    # parser
    src/parser/parser.c

    # expander
    src/expander/pipeline.c
    src/expander/expander.c

    # builtins
    src/builtin/builtin.c
    src/builtin/history.c
    src/builtin/job_control.c

    # executor
    src/executor/executor.c
    src/executor/jobs.c

    # history
    src/history/history.c

    # shell
    src/shell/shell.c
    src/shell/state.c
    src/shell/signal.c

    # prompt
    src/prompt/ps1.c

    # utils
    src/utils/system/syscall.c
    src/utils/system/memory.c
    src/utils/collections.c
    src/utils/utils.c
)

# -- PACKAGES --
find_package(PkgConfig REQUIRED)
pkg_check_modules(READLINE REQUIRED readline)

if (ENABLE_TESTS)
    pkg_check_modules(CRITERION REQUIRED criterion)
    if(CRITERION_FOUND)
        message(STATUS "Criterion found, enabling tests")
        set(HAVE_CRITERION TRUE)
    else()
        message(WARNING "Criterion not found, tests will be disabled")
        set(HAVE_CRITERION FALSE)
    endif()
endif()

# --- Build the core as a library ---
add_library(novash_core ${NOVASH_SOURCES})
target_compile_definitions(novash_core PUBLIC LOG_LEVEL=${LOG_LEVEL_INT})
target_include_directories(novash_core PUBLIC src external ${READLINE_INCLUDE_DIRS})
target_link_libraries(novash_core PUBLIC ${READLINE_LIBRARIES})
novash_target_enable_warnings(novash_core)

# --- Main executable (shell) ---
add_executable(nsh src/main.c)
target_compile_definitions(nsh PRIVATE LOG_LEVEL=${LOG_LEVEL_INT})
target_link_libraries(nsh PRIVATE novash_core)
novash_target_enable_warnings(nsh)

set(TEST_SOURCES
    tests/test_lexer.c
    tests/test_parser.c
    tests/test_expander.c
)

if (ENABLE_TESTS AND HAVE_CRITERION)
    add_executable(tests_novash ${TEST_SOURCES})
    target_compile_definitions(tests_novash PRIVATE LOG_LEVEL=${LOG_LEVEL_INT} "LOG_LEVEL=${LOG_LEVEL_INT}" "TEST=1")
    target_include_directories(tests_novash PRIVATE ${CRITERION_INCLUDE_DIRS} src)

    # Ensure the core library is also built with TEST defined so logging is disabled
    # in all compilation units, not only in the test executable.
    target_compile_definitions(novash_core PRIVATE "TEST=1")

    target_link_libraries(tests_novash PRIVATE novash_core ${CRITERION_LIBRARIES} ${READLINE_LIBRARIES})
    enable_testing()
    add_test(NAME NovashTests COMMAND tests_novash)
endif()

# -----------------------
# Build-type specific flags
# -----------------------
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(nsh PRIVATE
        $<$<CONFIG:Debug>:-g -O0>
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:RelWithDebInfo>:-O2 -g>
        $<$<CONFIG:MinSizeRel>:-Os>
    )
elseif(MSVC)
    target_compile_options(nsh PRIVATE
        $<$<CONFIG:Debug>:/Zi /Od>
        $<$<CONFIG:Release>:/O2 /DNDEBUG>
    )
endif()

# -----------------------
# Sanitizers
# -----------------------
if(ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE MATCHES Debug AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    message(STATUS "Enabling ASAN/UBSAN for Debug build")
    target_compile_options(nsh PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(nsh PRIVATE -fsanitize=address,undefined)
endif()
