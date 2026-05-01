#include "ccvm/optimizer/pass_manager.h"
#include "ccvm/optimizer/ir_types.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <float.h>

// Constant Folding pass
// Mengevaluasi constant expressions pada compile time

typedef struct const_fold_context {
    ccvm_function_t* function;
    size_t folded_count;        // Jumlah instructions yang difold
    size_t total_count;         // Total instructions yang diproses
    bool changed;               // Apakah ada perubahan
} const_fold_context_t;

// Fungsi untuk mengecek apakah value adalah constant
static bool is_constant(ccvm_value_t* value) {
    return ccvm_value_is_constant(value);
}

// Fungsi untuk mengecek apakah instruction bisa difold
static bool can_fold_instruction(ccvm_instruction_t* inst) {
    if (!inst) return false;
    
    enum ccvm_opcode opcode = ccvm_instruction_get_opcode(inst);
    
    // Hanya fold binary operations untuk sekarang
    switch (opcode) {
        case CCVM_OP_ADD:
        case CCVM_OP_SUB:
        case CCVM_OP_MUL:
        case CCVM_OP_UDIV:
        case CCVM_OP_SDIV:
        case CCVM_OP_UREM:
        case CCVM_OP_SREM:
        case CCVM_OP_SHL:
        case CCVM_OP_LSHR:
        case CCVM_OP_ASHR:
        case CCVM_OP_AND:
        case CCVM_OP_OR:
        case CCVM_OP_XOR:
            return true;
            
        default:
            return false;
    }
}

// Fungsi untuk mendapatkan constant integer value
static bool get_constant_int(ccvm_value_t* value, int64_t* result) {
    if (!is_constant(value) || !result) {
        return false;
    }
    
    // TODO: Implementasi pengecekan tipe yang lebih comprehensive
    *result = ccvm_constant_get_integer(value);
    return true;
}

// Fungsi untuk mendapatkan constant float value
static bool get_constant_float(ccvm_value_t* value, double* result) {
    if (!is_constant(value) || !result) {
        return false;
    }
    
    // TODO: Implementasi pengecekan tipe yang lebih comprehensive
    *result = ccvm_constant_get_float(value);
    return true;
}

// Fungsi untuk membuat constant value baru
static ccvm_value_t* create_constant_value(ccvm_type_t* type, int64_t int_val, double float_val) {
    // TODO: Implementasi pembuatan constant berdasarkan tipe
    if (ccvm_type_is_integer(type)) {
        return ccvm_constant_create_integer(type, int_val);
    } else if (ccvm_type_is_float(type)) {
        return ccvm_constant_create_float(type, float_val);
    }
    
    return NULL;
}

// Fungsi untuk melakukan constant folding pada binary operation
static ccvm_value_t* fold_binary_op(ccvm_instruction_t* inst) {
    if (!inst) return NULL;
    
    size_t operand_count = ccvm_instruction_get_operand_count(inst);
    if (operand_count < 2) return NULL;
    
    ccvm_value_t* op1 = ccvm_instruction_get_operand(inst, 0);
    ccvm_value_t* op2 = ccvm_instruction_get_operand(inst, 1);
    
    if (!op1 || !op2) return NULL;
    
    // Hanya fold jika kedua operands adalah constant
    if (!is_constant(op1) || !is_constant(op2)) {
        return NULL;
    }
    
    ccvm_type_t* result_type = ccvm_instruction_get_type(inst);
    if (!result_type) return NULL;
    
    enum ccvm_opcode opcode = ccvm_instruction_get_opcode(inst);
    
    // Lakukan folding berdasarkan tipe
    if (ccvm_type_is_integer(result_type)) {
        int64_t val1, val2, result = 0;
        
        if (!get_constant_int(op1, &val1) || !get_constant_int(op2, &val2)) {
            return NULL;
        }
        
        // Lakukan operasi integer
        switch (opcode) {
            case CCVM_OP_ADD:
                result = val1 + val2;
                break;
            case CCVM_OP_SUB:
                result = val1 - val2;
                break;
            case CCVM_OP_MUL:
                result = val1 * val2;
                break;
            case CCVM_OP_UDIV:
                if (val2 == 0) return NULL; // Division by zero
                result = (uint64_t)val1 / (uint64_t)val2;
                break;
            case CCVM_OP_SDIV:
                if (val2 == 0) return NULL; // Division by zero
                result = val1 / val2;
                break;
            case CCVM_OP_UREM:
                if (val2 == 0) return NULL; // Division by zero
                result = (uint64_t)val1 % (uint64_t)val2;
                break;
            case CCVM_OP_SREM:
                if (val2 == 0) return NULL; // Division by zero
                result = val1 % val2;
                break;
            case CCVM_OP_AND:
                result = val1 & val2;
                break;
            case CCVM_OP_OR:
                result = val1 | val2;
                break;
            case CCVM_OP_XOR:
                result = val1 ^ val2;
                break;
            case CCVM_OP_SHL:
                result = val1 << val2;
                break;
            case CCVM_OP_LSHR:
                result = (uint64_t)val1 >> val2;
                break;
            case CCVM_OP_ASHR:
                result = val1 >> val2;
                break;
            default:
                return NULL;
        }
        
        return ccvm_constant_create_integer(result_type, result);
        
    } else if (ccvm_type_is_float(result_type)) {
        double val1, val2, result = 0.0;
        
        if (!get_constant_float(op1, &val1) || !get_constant_float(op2, &val2)) {
            return NULL;
        }
        
        // Lakukan operasi float
        switch (opcode) {
            case CCVM_OP_ADD:
                result = val1 + val2;
                break;
            case CCVM_OP_SUB:
                result = val1 - val2;
                break;
            case CCVM_OP_MUL:
                result = val1 * val2;
                break;
            case CCVM_OP_SDIV: // Treat as float division
                if (val2 == 0.0) return NULL; // Division by zero
                result = val1 / val2;
                break;
            default:
                return NULL;
        }
        
        return ccvm_constant_create_float(result_type, result);
    }
    
    return NULL;
}

// Fungsi untuk memproses instruction
static bool process_instruction(ccvm_instruction_t* inst, const_fold_context_t* ctx) {
    if (!inst || !ctx) return false;
    
    ctx->total_count++;
    
    // Cek apakah instruction bisa difold
    if (!can_fold_instruction(inst)) {
        return false;
    }
    
    // Lakukan constant folding
    ccvm_value_t* folded_value = fold_binary_op(inst);
    
    if (folded_value) {
        ctx->folded_count++;
        ctx->changed = true;
        
        // TODO: Replace instruction with constant value
        // Ini memerlukan IR modification API yang belum diimplementasi
        
        return true;
    }
    
    return false;
}

// Fungsi untuk memproses basic block
static void process_basic_block(ccvm_basic_block_t* block, const_fold_context_t* ctx) {
    if (!block || !ctx) return;
    
    size_t inst_count = ccvm_basic_block_get_instruction_count(block);
    
    for (size_t i = 0; i < inst_count; i++) {
        ccvm_instruction_t* inst = ccvm_basic_block_get_instruction(block, i);
        process_instruction(inst, ctx);
    }
}

// Fungsi untuk memproses function
static void process_function(ccvm_function_t* function, const_fold_context_t* ctx) {
    if (!function || !ctx) return;
    
    size_t bb_count = ccvm_function_get_basic_block_count(function);
    
    for (size_t i = 0; i < bb_count; i++) {
        ccvm_basic_block_t* bb = ccvm_function_get_basic_block(function, i);
        process_basic_block(bb, ctx);
    }
}

// Main constant folding pass function
static enum ccvm_pass_result const_folding_run(ccvm_pass_t* pass, void* ir_unit) {
    if (!pass || !ir_unit) {
        return CCVM_PASS_INVALID_INPUT;
    }
    
    ccvm_function_t* function = (ccvm_function_t*)ir_unit;
    
    // Initialize context
    const_fold_context_t ctx = {0};
    ctx.function = function;
    ctx.changed = false;
    
    // Process function
    process_function(function, &ctx);
    
    // Store statistics
    pass->private_data = malloc(sizeof(const_fold_context_t));
    if (pass->private_data) {
        memcpy(pass->private_data, &ctx, sizeof(const_fold_context_t));
    }
    
    // Return appropriate result
    if (ctx.changed) {
        return CCVM_PASS_SUCCESS;
    } else if (ctx.total_count > 0) {
        return CCVM_PASS_NO_CHANGE;
    } else {
        return CCVM_PASS_SUCCESS;
    }
}

static void const_folding_initialize(ccvm_pass_t* pass) {
    // No initialization needed
}

static void const_folding_finalize(ccvm_pass_t* pass) {
    if (pass && pass->private_data) {
        free(pass->private_data);
        pass->private_data = NULL;
    }
}

static void const_folding_destroy(ccvm_pass_t* pass) {
    const_folding_finalize(pass);
}

static bool const_folding_is_required(ccvm_pass_t* pass, const void* ir_unit) {
    if (!pass || !ir_unit) {
        return false;
    }
    
    // Constant folding selalu berguna
    return true;
}

static void const_folding_get_statistics(ccvm_pass_t* pass, void* stats) {
    if (!pass || !pass->private_data || !stats) {
        return;
    }
    
    const_fold_context_t* ctx = (const_fold_context_t*)pass->private_data;
    
    // TODO: Copy statistics to output structure
    (void)ctx; // Suppress unused parameter warning
    (void)stats; // Suppress unused parameter warning
}

// Pass vtable
static const struct ccvm_pass_vtable const_folding_vtable = {
    .run = const_folding_run,
    .initialize = const_folding_initialize,
    .finalize = const_folding_finalize,
    .destroy = const_folding_destroy,
    .is_required = const_folding_is_required,
    .get_statistics = const_folding_get_statistics,
};

// Pass info
static const struct ccvm_pass_info const_folding_info = {
    .name = "const_folding",
    .description = "Constant folding - evaluate constant expressions at compile time",
    .type = CCVM_PASS_FUNCTION,
    .strategy = CCVM_PASS_ONCE,
    .is_analysis_pass = false,
    .preserves_all = false,
    .modifies_cfg = false,
    .dependency_count = 0,
    .dependencies = NULL,
};

// Create constant folding pass
ccvm_pass_t* ccvm_create_const_folding_pass(void) {
    ccvm_pass_t* pass = (ccvm_pass_t*)calloc(1, sizeof(ccvm_pass_t));
    if (!pass) {
        return NULL;
    }
    
    pass->vtable = &const_folding_vtable;
    pass->info = &const_folding_info;
    pass->is_enabled = true;
    pass->is_initialized = false;
    pass->execution_count = 0;
    pass->total_time_ms = 0;
    pass->last_result = CCVM_PASS_SUCCESS;
    
    return pass;
}