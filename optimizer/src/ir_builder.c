#include "ccvm/optimizer/optimizer.h"
#include "ccvm/optimizer/ir_types.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
// Minimal conservative IR builder implementations to reduce compile-time errors

ccvm_module_t* ccvm_module_create(const char* name) {
    ccvm_module_t* m = (ccvm_module_t*)calloc(1, sizeof(ccvm_module_t));
    if (!m) return NULL;
    if (name) {
        m->name = strdup(name);
    }
    m->function_count = 0;
    m->functions = NULL;
    m->global_count = 0;
    m->globals = NULL;
    return m;
}

void ccvm_module_destroy(ccvm_module_t* module) {
    if (!module) return;
    if (module->name) free((void*)module->name);
    free(module->functions);
    free(module->globals);
    free(module->types);
    free(module);
}

ccvm_function_t* ccvm_module_create_function(
    ccvm_module_t* module,
    const char* name,
    ccvm_type_t* type,
    bool is_variadic) {
    if (!module) return NULL;
    ccvm_function_t* f = (ccvm_function_t*)calloc(1, sizeof(ccvm_function_t));
    if (!f) return NULL;
    f->name = name ? strdup(name) : NULL;
    f->parent = module;
    f->type = type;
    f->basic_block_count = 0;
    f->basic_blocks = NULL;
    f->argument_count = 0;
    f->arguments = NULL;
    f->is_variadic = is_variadic;

    // Append to module's function list
    size_t new_count = module->function_count + 1;
    module->functions = (ccvm_function_t**)realloc(module->functions, sizeof(ccvm_function_t*) * new_count);
    module->functions[module->function_count] = f;
    module->function_count = new_count;

    return f;
}

ccvm_basic_block_t* ccvm_function_create_basic_block(
    ccvm_function_t* function,
    const char* name) {
    if (!function) return NULL;
    ccvm_basic_block_t* b = (ccvm_basic_block_t*)calloc(1, sizeof(ccvm_basic_block_t));
    if (!b) return NULL;
    b->name = name ? strdup(name) : NULL;
    b->parent = function;
    b->first_inst = b->last_inst = NULL;
    b->predecessor_count = 0;
    b->predecessors = NULL;
    b->successor_count = 0;
    b->successors = NULL;

    size_t new_count = function->basic_block_count + 1;
    function->basic_blocks = (ccvm_basic_block_t**)realloc(function->basic_blocks, sizeof(ccvm_basic_block_t*) * new_count);
    function->basic_blocks[function->basic_block_count] = b;
    function->basic_block_count = new_count;

    return b;
}

ccvm_instruction_t* ccvm_basic_block_create_instruction(
    ccvm_basic_block_t* block,
    enum ccvm_opcode opcode,
    ccvm_type_t* type,
    ccvm_value_t** operands,
    size_t operand_count) {
    if (!block) return NULL;
    ccvm_instruction_t* inst = (ccvm_instruction_t*)calloc(1, sizeof(ccvm_instruction_t));
    if (!inst) return NULL;
    inst->opcode = opcode;
    inst->operand_count = operand_count;
    if (operand_count && operands) {
        inst->operands = (ccvm_value_t**)malloc(sizeof(ccvm_value_t*) * operand_count);
        for (size_t i = 0; i < operand_count; i++) inst->operands[i] = operands[i];
    } else {
        inst->operands = NULL;
    }

    // Append to block's instruction list (simple linked list)
    if (!block->first_inst) {
        block->first_inst = block->last_inst = inst;
    } else {
        block->last_inst->next = inst;
        inst->prev = block->last_inst;
        block->last_inst = inst;
    }

    return inst;
}

ccvm_value_t* ccvm_basic_block_create_phi(
    ccvm_basic_block_t* block,
    ccvm_type_t* type,
    ccvm_basic_block_t** incoming_blocks,
    ccvm_value_t** incoming_values,
    size_t incoming_count) {
    // Represent PHI as an instruction value
    ccvm_instruction_t* inst = (ccvm_instruction_t*)calloc(1, sizeof(ccvm_instruction_t));
    if (!inst) return NULL;
    inst->opcode = CCVM_OP_PHI;
    inst->operand_count = incoming_count;
    inst->operands = NULL;
    // Append to block
    if (block) {
        if (!block->first_inst) block->first_inst = block->last_inst = inst;
        else {
            block->last_inst->next = inst;
            inst->prev = block->last_inst;
            block->last_inst = inst;
        }
    }
    return (ccvm_value_t*)inst;
}

// Basic type and constant factories (minimal)
ccvm_type_t* ccvm_type_create_void(void) {
    ccvm_type_t* t = (ccvm_type_t*)calloc(1, sizeof(ccvm_type_t));
    if (!t) return NULL;
    t->type = CCVM_VALUE_VOID;
    return t;
}

ccvm_type_t* ccvm_type_create_integer(enum ccvm_value_type type, uint32_t bit_width) {
    ccvm_type_t* t = (ccvm_type_t*)calloc(1, sizeof(ccvm_type_t));
    if (!t) return NULL;
    t->type = type;
    t->bit_width = bit_width;
    return t;
}

ccvm_type_t* ccvm_type_create_float(enum ccvm_value_type type) {
    ccvm_type_t* t = (ccvm_type_t*)calloc(1, sizeof(ccvm_type_t));
    if (!t) return NULL;
    t->type = type;
    return t;
}

ccvm_type_t* ccvm_type_create_pointer(ccvm_type_t* element_type) {
    ccvm_type_t* t = (ccvm_type_t*)calloc(1, sizeof(ccvm_type_t));
    if (!t) return NULL;
    t->type = CCVM_VALUE_POINTER;
    t->element_type = element_type;
    return t;
}

ccvm_type_t* ccvm_type_create_array(ccvm_type_t* element_type, size_t size) {
    ccvm_type_t* t = (ccvm_type_t*)calloc(1, sizeof(ccvm_type_t));
    if (!t) return NULL;
    t->type = CCVM_VALUE_ARRAY;
    t->element_type = element_type;
    t->array_size = size;
    return t;
}

ccvm_type_t* ccvm_type_create_vector(ccvm_type_t* element_type, size_t size) {
    return ccvm_type_create_array(element_type, size);
}

ccvm_type_t* ccvm_type_create_function(
    ccvm_type_t* return_type,
    ccvm_type_t** param_types,
    size_t param_count,
    bool is_variadic) {
    ccvm_type_t* t = (ccvm_type_t*)calloc(1, sizeof(ccvm_type_t));
    if (!t) return NULL;
    t->type = CCVM_VALUE_FUNCTION;
    t->return_type = return_type;
    t->param_count = param_count;
    t->param_types = param_types;
    return t;
}

ccvm_type_t* ccvm_type_create_struct(
    const char* name,
    ccvm_type_t** field_types,
    size_t field_count,
    bool is_packed) {
    ccvm_type_t* t = (ccvm_type_t*)calloc(1, sizeof(ccvm_type_t));
    if (!t) return NULL;
    t->type = CCVM_VALUE_STRUCT;
    t->param_count = field_count;
    t->param_types = field_types;
    return t;
}

ccvm_value_t* ccvm_constant_create_integer(ccvm_type_t* type, int64_t value) {
    ccvm_value_t* v = (ccvm_value_t*)calloc(1, sizeof(ccvm_value_t));
    if (!v) return NULL;
    v->kind = CCVM_VALUE_CONSTANT_INT;
    v->type = type;
    v->id = (uint64_t)value;
    return v;
}

ccvm_value_t* ccvm_constant_create_float(ccvm_type_t* type, double value) {
    ccvm_value_t* v = (ccvm_value_t*)calloc(1, sizeof(ccvm_value_t));
    if (!v) return NULL;
    v->kind = CCVM_VALUE_CONSTANT_FP;
    v->type = type;
    // store double in metadata pointer as minimal approach
    double* d = (double*)malloc(sizeof(double)); *d = value; v->metadata = d;
    return v;
}

ccvm_value_t* ccvm_constant_create_null(ccvm_type_t* type) {
    ccvm_value_t* v = (ccvm_value_t*)calloc(1, sizeof(ccvm_value_t));
    if (!v) return NULL;
    v->kind = CCVM_VALUE_CONSTANT_POINTER_NULL;
    v->type = type;
    return v;
}

ccvm_value_t* ccvm_constant_create_undef(ccvm_type_t* type) {
    ccvm_value_t* v = (ccvm_value_t*)calloc(1, sizeof(ccvm_value_t));
    if (!v) return NULL;
    v->kind = CCVM_VALUE_UNDEF_VALUE;
    v->type = type;
    return v;
}

ccvm_value_t* ccvm_constant_create_array(
    ccvm_type_t* type,
    ccvm_value_t** elements,
    size_t element_count) {
    ccvm_value_t* v = (ccvm_value_t*)calloc(1, sizeof(ccvm_value_t));
    if (!v) return NULL;
    v->kind = CCVM_VALUE_CONSTANT_DATA_ARRAY;
    v->type = type;
    return v;
}

ccvm_value_t* ccvm_constant_create_struct(
    ccvm_type_t* type,
    ccvm_value_t** elements,
    size_t element_count) {
    ccvm_value_t* v = (ccvm_value_t*)calloc(1, sizeof(ccvm_value_t));
    if (!v) return NULL;
    v->kind = CCVM_VALUE_CONSTANT_DATA_VECTOR;
    v->type = type;
    return v;
}

const char* ccvm_opcode_to_string(enum ccvm_opcode opcode) {
    return "opcode";
}

const char* ccvm_type_to_string(ccvm_type_t* type) {
    return "type";
}

const char* ccvm_value_to_string(ccvm_value_t* value) {
    return "value";
}

bool ccvm_type_is_integer(ccvm_type_t* type) { return type && (type->type >= CCVM_VALUE_INT8 && type->type <= CCVM_VALUE_UINT64); }
bool ccvm_type_is_float(ccvm_type_t* type) { return type && (type->type == CCVM_VALUE_FLOAT32 || type->type == CCVM_VALUE_FLOAT64); }
bool ccvm_type_is_pointer(ccvm_type_t* type) { return type && type->type == CCVM_VALUE_POINTER; }
bool ccvm_type_is_array(ccvm_type_t* type) { return type && type->type == CCVM_VALUE_ARRAY; }
bool ccvm_type_is_vector(ccvm_type_t* type) { return ccvm_type_is_array(type); }
bool ccvm_type_is_function(ccvm_type_t* type) { return type && type->type == CCVM_VALUE_FUNCTION; }

uint32_t ccvm_type_get_bit_width(ccvm_type_t* type) { return type ? type->bit_width : 0; }
size_t ccvm_type_get_size(ccvm_type_t* type) { return type ? (type->bit_width/8) : 0; }
size_t ccvm_type_get_alignment(ccvm_type_t* type) { return 1; }

bool ccvm_instruction_is_terminator(ccvm_instruction_t* inst) { return inst && (inst->opcode == CCVM_OP_RET || inst->opcode == CCVM_OP_BR || inst->opcode == CCVM_OP_UNREACHABLE); }
bool ccvm_instruction_is_binary_op(ccvm_instruction_t* inst) { return inst && (inst->opcode >= CCVM_OP_ADD && inst->opcode <= CCVM_OP_SREM); }
bool ccvm_instruction_is_memory_op(ccvm_instruction_t* inst) { return inst && (inst->opcode == CCVM_OP_LOAD || inst->opcode == CCVM_OP_STORE || inst->opcode == CCVM_OP_ALLOCA); }
bool ccvm_instruction_is_cast(ccvm_instruction_t* inst) { return inst && (inst->opcode >= CCVM_OP_TRUNC && inst->opcode <= CCVM_OP_ADDRSPACECAST); }
bool ccvm_instruction_is_commutative(ccvm_instruction_t* inst) { return inst && (inst->opcode == CCVM_OP_ADD || inst->opcode == CCVM_OP_MUL || inst->opcode == CCVM_OP_AND || inst->opcode == CCVM_OP_OR || inst->opcode == CCVM_OP_XOR); }
bool ccvm_instruction_may_have_side_effects(ccvm_instruction_t* inst) { return inst && (inst->opcode == CCVM_OP_STORE || inst->opcode == CCVM_OP_CALL || inst->opcode == CCVM_OP_INVOKE); }
bool ccvm_instruction_may_read_memory(ccvm_instruction_t* inst) { return inst && (inst->opcode == CCVM_OP_LOAD || inst->opcode == CCVM_OP_CALL); }
bool ccvm_instruction_may_write_memory(ccvm_instruction_t* inst) { return inst && (inst->opcode == CCVM_OP_STORE || inst->opcode == CCVM_OP_CALL); }

bool ccvm_value_is_constant(ccvm_value_t* value) { return value && (value->kind == CCVM_VALUE_CONSTANT_INT || value->kind == CCVM_VALUE_CONSTANT_FP || value->kind == CCVM_VALUE_CONSTANT_POINTER_NULL); }
bool ccvm_value_is_undef(ccvm_value_t* value) { return value && value->kind == CCVM_VALUE_UNDEF_VALUE; }
bool ccvm_value_is_null(ccvm_value_t* value) { return value && value->kind == CCVM_VALUE_CONSTANT_POINTER_NULL; }

int64_t ccvm_constant_get_integer(ccvm_value_t* constant) { return constant ? (int64_t)constant->id : 0; }
double ccvm_constant_get_float(ccvm_value_t* constant) { return constant && constant->metadata ? *(double*)constant->metadata : 0.0; }

ccvm_type_t* ccvm_value_get_type(ccvm_value_t* value) { return value ? value->type : NULL; }

size_t ccvm_module_get_function_count(ccvm_module_t* module) { return module ? module->function_count : 0; }
ccvm_function_t* ccvm_module_get_function(ccvm_module_t* module, size_t index) { return (module && index < module->function_count) ? module->functions[index] : NULL; }
ccvm_function_t* ccvm_module_get_function_by_name(ccvm_module_t* module, const char* name) {
    if (!module || !name) return NULL;
    for (size_t i = 0; i < module->function_count; i++) {
        if (module->functions[i] && module->functions[i]->name && strcmp(module->functions[i]->name, name) == 0) return module->functions[i];
    }
    return NULL;
}

size_t ccvm_module_get_global_count(ccvm_module_t* module) { return module ? module->global_count : 0; }
ccvm_value_t* ccvm_module_get_global(ccvm_module_t* module, size_t index) { return (module && index < module->global_count) ? module->globals[index] : NULL; }

size_t ccvm_function_get_basic_block_count(ccvm_function_t* function) { return function ? function->basic_block_count : 0; }
ccvm_basic_block_t* ccvm_function_get_basic_block(ccvm_function_t* function, size_t index) { return (function && index < function->basic_block_count) ? function->basic_blocks[index] : NULL; }
ccvm_basic_block_t* ccvm_function_get_basic_block_by_name(ccvm_function_t* function, const char* name) {
    if (!function || !name) return NULL;
    for (size_t i = 0; i < function->basic_block_count; i++) if (function->basic_blocks[i] && function->basic_blocks[i]->name && strcmp(function->basic_blocks[i]->name,name)==0) return function->basic_blocks[i];
    return NULL;
}

size_t ccvm_function_get_argument_count(ccvm_function_t* function) { return function ? function->argument_count : 0; }
ccvm_value_t* ccvm_function_get_argument(ccvm_function_t* function, size_t index) { return (function && index < function->argument_count) ? function->arguments[index] : NULL; }

size_t ccvm_basic_block_get_instruction_count(ccvm_basic_block_t* block) {
    if (!block) return 0;
    size_t cnt = 0; ccvm_instruction_t* it = block->first_inst; while (it) { cnt++; it = it->next; } return cnt;
}
ccvm_instruction_t* ccvm_basic_block_get_instruction(ccvm_basic_block_t* block, size_t index) { if (!block) return NULL; size_t i=0; ccvm_instruction_t* it=block->first_inst; while (it) { if (i==index) return it; i++; it=it->next; } return NULL; }
ccvm_instruction_t* ccvm_basic_block_get_terminator(ccvm_basic_block_t* block) { if (!block) return NULL; return block->terminator; }

size_t ccvm_basic_block_get_predecessor_count(ccvm_basic_block_t* block) { return block ? block->predecessor_count : 0; }
ccvm_basic_block_t* ccvm_basic_block_get_predecessor(ccvm_basic_block_t* block, size_t index) { return (block && index < block->predecessor_count) ? block->predecessors[index] : NULL; }
size_t ccvm_basic_block_get_successor_count(ccvm_basic_block_t* block) { return block ? block->successor_count : 0; }
ccvm_basic_block_t* ccvm_basic_block_get_successor(ccvm_basic_block_t* block, size_t index) { return (block && index < block->successor_count) ? block->successors[index] : NULL; }

bool ccvm_module_verify(ccvm_module_t* module, char** error_message) { (void)error_message; return module != NULL; }
bool ccvm_function_verify(ccvm_function_t* function, char** error_message) { (void)error_message; return function != NULL; }
bool ccvm_basic_block_verify(ccvm_basic_block_t* block, char** error_message) { (void)error_message; return block != NULL; }
bool ccvm_instruction_verify(ccvm_instruction_t* instruction, char** error_message) { (void)error_message; return instruction != NULL; }

void ccvm_module_print(ccvm_module_t* module, FILE* file) { if (!file) file = stdout; fprintf(file, "Module: %s\n", module ? (module->name?module->name:"<anon>") : "<null>"); }
void ccvm_function_print(ccvm_function_t* function, FILE* file) { if (!file) file = stdout; fprintf(file, "Function: %s\n", function? (function->name?function->name:"<anon>"):"<null>"); }
void ccvm_basic_block_print(ccvm_basic_block_t* block, FILE* file) { if (!file) file = stdout; fprintf(file, "BasicBlock: %s\n", block? (block->name?block->name:"<anon>"):"<null>"); }
void ccvm_instruction_print(ccvm_instruction_t* instruction, FILE* file) { if (!file) file = stdout; fprintf(file, "Instruction opcode=%d\n", instruction? (int)instruction->opcode : -1); }
void ccvm_value_print(ccvm_value_t* value, FILE* file) { if (!file) file = stdout; fprintf(file, "Value kind=%d\n", value? (int)value->kind : -1); }