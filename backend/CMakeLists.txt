# CCVM Backend - CMake Configuration
cmake_minimum_required(VERSION 3.16)
project(ccvm_backend C)

set(CMAKE_C_STANDARD 11)

include_directories(include)

set(BACKEND_SOURCES
    src/codegen.c
    src/ir_parser.c
    src/x86_64/isel.c
    src/arm64/isel.c
    src/riscv/isel.c
)

add_library(ccvm_backend STATIC ${BACKEND_SOURCES})
