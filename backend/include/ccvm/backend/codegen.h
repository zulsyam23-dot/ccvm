/**
 * @file codegen.h
 * @brief Universal Code Generation interface for CCVM
 * @author CCVM Team
 * @date 2026
 */

#ifndef CCVM_BACKEND_CODEGEN_H
#define CCVM_BACKEND_CODEGEN_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Target architectures supported by CCVM
 */
typedef enum {
    CCVM_TARGET_X86_64,
    CCVM_TARGET_ARM64,
    CCVM_TARGET_RISCV,
    CCVM_TARGET_WASM
} ccvm_target_arch_t;

/**
 * @brief Backend configuration
 */
struct ccvm_backend_config {
    ccvm_target_arch_t arch;
    int optimization_level;
    bool debug_info;
};

/**
 * @brief Generate machine code from CCVM IR
 * 
 * @param ir_module Pointer to the IR module structure
 * @param config Backend configuration
 * @param output_file Path to the output binary/assembly file
 * @return int 1 on success, 0 on failure
 */
int ccvm_backend_generate(void* ir_module, const struct ccvm_backend_config* config, const char* output_file);

/**
 * @brief Compile textual IR file to target output
 * 
 * @param ir_file Path to .ir file
 * @param config Backend configuration
 * @param output_file Path to the output binary/assembly file
 * @return int 1 on success, 0 on failure
 */
int ccvm_backend_compile(const char* ir_file, const struct ccvm_backend_config* config, const char* output_file);

#ifdef __cplusplus
}
#endif

#endif
