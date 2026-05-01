/**
 * @file codegen.c
 * @brief Code generation dispatcher
 * @author CCVM Team
 * @date 2026
 */

#include "ccvm/backend/codegen.h"
#include "ccvm/backend/ir.h"
#include "ccvm/backend/ir_parser.h"
#include <stdio.h>
#include <string.h>

/* Forward declarations for target backends */
int ccvm_x86_64_generate(const ccvm_ir_module_t* module, const char* output_file);

int ccvm_backend_generate(void* ir_module, const struct ccvm_backend_config* config, const char* output_file) {
    if (!ir_module || !config || !output_file) return 0;

    ccvm_ir_module_t* module = (ccvm_ir_module_t*)ir_module;

    switch (config->arch) {
        case CCVM_TARGET_X86_64:
            return ccvm_x86_64_generate(module, output_file);
        case CCVM_TARGET_ARM64:
            fprintf(stderr, "ARM64 backend not yet implemented\n");
            return 0;
        case CCVM_TARGET_RISCV:
            fprintf(stderr, "RISC-V backend not yet implemented\n");
            return 0;
        default:
            fprintf(stderr, "Unsupported architecture\n");
            return 0;
    }
}

int ccvm_backend_compile(const char* ir_file, const struct ccvm_backend_config* config, const char* output_file) {
    if (!ir_file || !config || !output_file) return 0;

    ccvm_ir_module_t module;
    if (!ccvm_ir_parse(ir_file, &module)) {
        fprintf(stderr, "Error: Failed to parse IR file: %s\n", ir_file);
        return 0;
    }

    return ccvm_backend_generate(&module, config, output_file);
}
