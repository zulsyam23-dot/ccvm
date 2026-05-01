# CCVM Optimizer - CMake Configuration
# Optimizer intermediate dalam C untuk performa tinggi

cmake_minimum_required(VERSION 3.16)
project(ccvm_optimizer C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags untuk performa tinggi
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -march=native -mtune=native")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -funroll-loops -finline-functions")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffast-math -fno-strict-aliasing")
    
    # Debug flags
    set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DDEBUG -fsanitize=address,undefined")
    
    # Release flags
    set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG -flto")
    
elseif(MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /O2 /arch:AVX2")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")
    
    # Debug flags
    set(CMAKE_C_FLAGS_DEBUG "/Od /DDEBUG /Zi")
    
    # Release flags
    set(CMAKE_C_FLAGS_RELEASE "/O2 /DNDEBUG /GL")
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/include
)

# Source files
set(OPTIMIZER_SOURCES
    src/optimizer.c
    src/pass_manager.c
    src/analysis.c
    src/transforms.c
    src/mem2reg.c
    src/dce.c
    src/const_folding.c
    src/inlining.c
    src/loop_opt.c
    src/vectorization.c
    src/alias_analysis.c
    src/dom_tree.c
    src/loop_analysis.c
    src/scalar_evolution.c
    src/value_tracking.c
    src/ir_builder.c
    src/ir_printer.c
    src/ir_verifier.c
    src/optimization_stats.c
    src/profile_guided.c
    src/interprocedural.c
)

# Header files
set(OPTIMIZER_HEADERS
    include/ccvm/optimizer/optimizer.h
    include/ccvm/optimizer/pass_manager.h
    include/ccvm/optimizer/analysis.h
    include/ccvm/optimizer/transforms.h
    include/ccvm/optimizer/passes.h
    include/ccvm/optimizer/ir_types.h
    include/ccvm/optimizer/dom_tree.h
    include/ccvm/optimizer/loop_analysis.h
    include/ccvm/optimizer/alias_analysis.h
    include/ccvm/optimizer/scalar_evolution.h
    include/ccvm/optimizer/value_tracking.h
    include/ccvm/optimizer/ir_builder.h
    include/ccvm/optimizer/ir_printer.h
    include/ccvm/optimizer/ir_verifier.h
    include/ccvm/optimizer/optimization_stats.h
    include/ccvm/optimizer/profile_guided.h
    include/ccvm/optimizer/interprocedural.h
    include/ccvm/optimizer/config.h
)

# Create static library
add_library(ccvm_optimizer STATIC ${OPTIMIZER_SOURCES} ${OPTIMIZER_HEADERS})

# Set library properties
set_target_properties(ccvm_optimizer PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "${OPTIMIZER_HEADERS}"
)

# Link dengan core library (jika ada)
if(TARGET ccvm_core)
    target_link_libraries(ccvm_optimizer PRIVATE ccvm_core)
endif()

# Library dependencies
find_package(Threads REQUIRED)
target_link_libraries(ccvm_optimizer PRIVATE Threads::Threads)

# Optional: Intel TBB untuk parallel processing
find_package(TBB QUIET)
if(TBB_FOUND)
    target_link_libraries(ccvm_optimizer PRIVATE TBB::tbb)
    target_compile_definitions(ccvm_optimizer PRIVATE CCVM_USE_TBB)
endif()

# Optional: OpenMP untuk parallel loops
find_package(OpenMP)
if(OpenMP_C_FOUND)
    target_link_libraries(ccvm_optimizer PRIVATE OpenMP::OpenMP_C)
    target_compile_definitions(ccvm_optimizer PRIVATE CCVM_USE_OPENMP)
endif()

# Build executable untuk testing
add_executable(opt_test test/opt_test.c)
target_link_libraries(opt_test PRIVATE ccvm_optimizer)

# Build benchmark executable
add_executable(opt_bench benchmark/opt_bench.c)
target_link_libraries(opt_bench PRIVATE ccvm_optimizer)

# Testing
enable_testing()
add_test(NAME optimizer_test COMMAND opt_test)

# Install rules
install(TARGETS ccvm_optimizer
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include/ccvm/optimizer
)

# Install additional headers
install(FILES ${OPTIMIZER_HEADERS}
    DESTINATION include/ccvm/optimizer
)

# Package configuration
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ccvm_optimizerConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ccvm_optimizerConfigVersion.cmake"
    DESTINATION lib/cmake/ccvm_optimizer
)