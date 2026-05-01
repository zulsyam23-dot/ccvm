cmake_minimum_required(VERSION 3.20)
project(ccvm-frontend VERSION 0.1.0 LANGUAGES CXX C)

# C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
    add_compile_options(-O3 -march=native)
elseif(MSVC)
    add_compile_options(/W4)
    add_compile_options(-D_CRT_SECURE_NO_WARNINGS)
    # Use static runtime
    foreach(flag_var
        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE)
        if(${flag_var} MATCHES "/MD")
            string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        endif()
    endforeach()
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(/O2)
    endif()
endif()

# Dependencies
find_package(Threads REQUIRED)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../backend/include)

# Backend sources
set(BACKEND_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../backend/src/codegen.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../backend/src/ir_parser.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../backend/src/x86_64/isel.c
)

# Source files
set(FRONTEND_SOURCES
    src/lexer.cpp
    src/parser.cpp
    src/ast.cpp
    src/semantic_analyzer.cpp
    src/ir_generator.cpp
    src/error_handler.cpp
    src/token.cpp
    src/location.cpp
    ${BACKEND_SOURCES}
)

# Header files
set(FRONTEND_HEADERS
    include/ccvm/frontend/lexer.h
    include/ccvm/frontend/parser.h
    include/ccvm/frontend/ast.h
    include/ccvm/frontend/semantic_analyzer.h
    include/ccvm/frontend/ir_generator.h
    include/ccvm/frontend/error_handler.h
    include/ccvm/frontend/token.h
    include/ccvm/frontend/location.h
    include/ccvm/frontend/common.h
)

# Library
add_library(ccvm_frontend STATIC ${FRONTEND_SOURCES} ${FRONTEND_HEADERS})
target_link_libraries(ccvm_frontend PUBLIC Threads::Threads)

# Export targets for installation
install(TARGETS ccvm_frontend
    EXPORT ccvm_frontend_targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

# Executable
add_executable(ccvm-frontend-cli src/main.cpp)
target_link_libraries(ccvm-frontend-cli PRIVATE ccvm_frontend)

# Tests
install(EXPORT ccvm_frontend_targets
    FILE ccvm-frontend-targets.cmake
    NAMESPACE ccvm::
    DESTINATION lib/cmake/ccvm-frontend
)

# Package config
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ccvm-frontend-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ccvm-frontend-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ccvm-frontend-config.cmake"
    INSTALL_DESTINATION lib/cmake/ccvm-frontend
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ccvm-frontend-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/ccvm-frontend-config-version.cmake"
    DESTINATION lib/cmake/ccvm-frontend
)