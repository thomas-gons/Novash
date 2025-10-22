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
    src/main.c

    # executor
    src/executor/builtin.c
    src/executor/executor.c
    src/executor/jobs.c

    # history
    src/history/history.c

    # parser
    src/parser/parser.c

    # shell
    src/shell/shell.c
    src/shell/state.c
    src/shell/signal.c

    # tokenizer
    src/tokenizer/tokenizer.c

    # utils
    src/utils/collections.c
    src/utils/memory.c
    src/utils/utils.c
)

# -----------------------
# Executable target
# -----------------------
add_executable(nsh ${NOVASH_SOURCES})
target_include_directories(nsh PRIVATE src /usr/include/readline)
target_link_libraries(nsh PRIVATE readline)
novash_target_enable_warnings(nsh)

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

# -----------------------
# Convenience targets
# -----------------------
add_custom_target(format
    COMMAND clang-format -i ${NOVASH_SOURCES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Format source files with clang-format"
)
