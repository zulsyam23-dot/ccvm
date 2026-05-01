#ifndef CCVM_OPTIMIZER_IR_TYPES_H
#define CCVM_OPTIMIZER_IR_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Forward declarations
typedef struct ccvm_type ccvm_type_t;
typedef struct ccvm_value ccvm_value_t;
typedef struct ccvm_instruction ccvm_instruction_t;
typedef struct ccvm_basic_block ccvm_basic_block_t;
typedef struct ccvm_function ccvm_function_t;
typedef struct ccvm_module ccvm_module_t;

// Value types
enum ccvm_value_type {
    CCVM_VALUE_INVALID = 0,
    CCVM_VALUE_VOID,
    CCVM_VALUE_BOOL,
    CCVM_VALUE_INT8,
    CCVM_VALUE_INT16,
    CCVM_VALUE_INT32,
    CCVM_VALUE_INT64,
    CCVM_VALUE_UINT8,
    CCVM_VALUE_UINT16,
    CCVM_VALUE_UINT32,
    CCVM_VALUE_UINT64,
    CCVM_VALUE_FLOAT32,
    CCVM_VALUE_FLOAT64,
    CCVM_VALUE_POINTER,
    CCVM_VALUE_ARRAY,
    CCVM_VALUE_STRUCT,
    CCVM_VALUE_FUNCTION,
    CCVM_VALUE_LABEL,
    CCVM_VALUE_METADATA,
};

// Instruction opcodes
enum ccvm_opcode {
    // Terminator instructions
    CCVM_OP_RET = 0,
    CCVM_OP_BR,
    CCVM_OP_SWITCH,
    CCVM_OP_INDIRECT_BR,
    CCVM_OP_INVOKE,
    CCVM_OP_RESUME,
    CCVM_OP_UNREACHABLE,
    
    // Binary operations
    CCVM_OP_ADD,
    CCVM_OP_SUB,
    CCVM_OP_MUL,
    CCVM_OP_UDIV,
    CCVM_OP_SDIV,
    CCVM_OP_UREM,
    CCVM_OP_SREM,
    
    // Logical operations
    CCVM_OP_SHL,
    CCVM_OP_LSHR,
    CCVM_OP_ASHR,
    CCVM_OP_AND,
    CCVM_OP_OR,
    CCVM_OP_XOR,
    
    // Memory operations
    CCVM_OP_ALLOCA,
    CCVM_OP_LOAD,
    CCVM_OP_STORE,
    CCVM_OP_GETELEMENTPTR,
    CCVM_OP_FENCE,
    CCVM_OP_ATOMICCMPXCHG,
    CCVM_OP_ATOMICRMW,
    
    // Conversion operations
    CCVM_OP_TRUNC,
    CCVM_OP_ZEXT,
    CCVM_OP_SEXT,
    CCVM_OP_FPTOUI,
    CCVM_OP_FPTOSI,
    CCVM_OP_UITOFP,
    CCVM_OP_SITOFP,
    CCVM_OP_FPTRUNC,
    CCVM_OP_FPEXT,
    CCVM_OP_PTRTOINT,
    CCVM_OP_INTTOPTR,
    CCVM_OP_BITCAST,
    CCVM_OP_ADDRSPACECAST,
    
    // Comparison operations
    CCVM_OP_ICMP,
    CCVM_OP_FCMP,
    
    // PHI and select
    CCVM_OP_PHI,
    CCVM_OP_SELECT,
    CCVM_OP_CALL,
    CCVM_OP_VAARG,
    CCVM_OP_LANDINGPAD,
    CCVM_OP_CATCHPAD,
    CCVM_OP_CLEANUPPAD,
    
    // Other operations
    CCVM_OP_EXTRACTELEMENT,
    CCVM_OP_INSERTELEMENT,
    CCVM_OP_SHUFFLEVECTOR,
    CCVM_OP_EXTRACTVALUE,
    CCVM_OP_INSERTVALUE,
    
    // Aggregate operations
    CCVM_OP_LSHR_EXACT,
    CCVM_OP_ASHR_EXACT,
    CCVM_OP_SHL_EXACT,
    
    // Maximum opcode value
    CCVM_OP_MAX,
};

// Comparison predicates
enum ccvm_icmp_predicate {
    CCVM_ICMP_EQ = 32,
    CCVM_ICMP_NE,
    CCVM_ICMP_UGT,
    CCVM_ICMP_UGE,
    CCVM_ICMP_ULT,
    CCVM_ICMP_ULE,
    CCVM_ICMP_SGT,
    CCVM_ICMP_SGE,
    CCVM_ICMP_SLT,
    CCVM_ICMP_SLE,
};

enum ccvm_fcmp_predicate {
    CCVM_FCMP_FALSE = 0,
    CCVM_FCMP_OEQ,
    CCVM_FCMP_OGT,
    CCVM_FCMP_OGE,
    CCVM_FCMP_OLT,
    CCVM_FCMP_OLE,
    CCVM_FCMP_ONE,
    CCVM_FCMP_ORD,
    CCVM_FCMP_UNO,
    CCVM_FCMP_UEQ,
    CCVM_FCMP_UGT,
    CCVM_FCMP_UGE,
    CCVM_FCMP_ULT,
    CCVM_FCMP_ULE,
    CCVM_FCMP_UNE,
    CCVM_FCMP_TRUE,
};

// Value kinds
enum ccvm_value_kind {
    CCVM_VALUE_ARGUMENT,
    CCVM_VALUE_BASIC_BLOCK,
    CCVM_VALUE_FUNCTION,
    CCVM_VALUE_GLOBAL_ALIAS,
    CCVM_VALUE_GLOBAL_VARIABLE,
    CCVM_VALUE_UNDEF_VALUE,
    CCVM_VALUE_BLOCK_ADDRESS,
    CCVM_VALUE_CONSTANT_EXPR,
    CCVM_VALUE_CONSTANT_AGGREGATE_ZERO,
    CCVM_VALUE_CONSTANT_DATA_ARRAY,
    CCVM_VALUE_CONSTANT_DATA_VECTOR,
    CCVM_VALUE_CONSTANT_INT,
    CCVM_VALUE_CONSTANT_FP,
    CCVM_VALUE_CONSTANT_POINTER_NULL,
    CCVM_VALUE_CONSTANT_TOKEN_NONE,
    CCVM_VALUE_METADATA_AS_VALUE,
    CCVM_VALUE_INLINE_ASM,
    CCVM_VALUE_INSTRUCTION,
    CCVM_VALUE_POISON_VALUE,
    CCVM_VALUE_CONSTANT_RANGE,
};

// Instruction flags
enum ccvm_instruction_flags {
    CCVM_INST_NONE = 0,
    CCVM_INST_NSW = 1 << 0, // No Signed Wrap
    CCVM_INST_NUW = 1 << 1, // No Unsigned Wrap
    CCVM_INST_EXACT = 1 << 2, // Exact division/shift
    CCVM_INST_VOLATILE = 1 << 3, // Volatile memory access
    CCVM_INST_ATOMIC = 1 << 4, // Atomic operation
    CCVM_INST_READONLY = 1 << 5, // Read-only memory access
    CCVM_INST_READWRITE = 1 << 6, // Read-write memory access
};

// Type information
struct ccvm_type_info {
    enum ccvm_value_type type;
    uint32_t bit_width;
    uint32_t alignment;
    bool is_const;
    bool is_volatile;
    bool is_signed;
    size_t array_size;
    size_t vector_size;
    ccvm_type_t* element_type;
    ccvm_type_t* return_type;
    size_t param_count;
    ccvm_type_t** param_types;
};

// Value structure
struct ccvm_value {
    ccvm_value_t* next;
    ccvm_value_t* prev;
    ccvm_type_t* type;
    const char* name;
    enum ccvm_value_kind kind;
    uint32_t flags;
    uint64_t id;
    void* metadata;
};

// Instruction structure
struct ccvm_instruction {
    struct ccvm_value base; // Base value
    ccvm_instruction_t* next;
    ccvm_instruction_t* prev;
    ccvm_basic_block_t* parent;
    enum ccvm_opcode opcode;
    size_t operand_count;
    ccvm_value_t** operands;
    uint32_t flags;
    void* analysis_info;
};

// Basic block structure
struct ccvm_basic_block {
    const char* name;
    ccvm_function_t* parent;
    ccvm_instruction_t* first_inst;
    ccvm_instruction_t* last_inst;
    ccvm_instruction_t* terminator;
    size_t predecessor_count;
    ccvm_basic_block_t** predecessors;
    size_t successor_count;
    ccvm_basic_block_t** successors;
    uint64_t id;
    void* analysis_info;
};

// Function structure
struct ccvm_function {
    const char* name;
    ccvm_module_t* parent;
    ccvm_type_t* type;
    size_t basic_block_count;
    ccvm_basic_block_t** basic_blocks;
    ccvm_basic_block_t* entry_block;
    size_t argument_count;
    ccvm_value_t** arguments;
    uint32_t flags;
    void* analysis_info;
    bool is_declaration;
    bool is_variadic;
};

// Module structure
struct ccvm_module {
    const char* name;
    size_t function_count;
    ccvm_function_t** functions;
    size_t global_count;
    ccvm_value_t** globals;
    size_t type_count;
    ccvm_type_t** types;
    void* metadata;
    void* analysis_info;
};

// IR creation functions
ccvm_module_t* ccvm_module_create(const char* name);
void ccvm_module_destroy(ccvm_module_t* module);
ccvm_function_t* ccvm_module_create_function(
    ccvm_module_t* module,
    const char* name,
    ccvm_type_t* type,
    bool is_variadic
);
ccvm_basic_block_t* ccvm_function_create_basic_block(
    ccvm_function_t* function,
    const char* name
);
ccvm_instruction_t* ccvm_basic_block_create_instruction(
    ccvm_basic_block_t* block,
    enum ccvm_opcode opcode,
    ccvm_type_t* type,
    ccvm_value_t** operands,
    size_t operand_count
);
ccvm_value_t* ccvm_basic_block_create_phi(
    ccvm_basic_block_t* block,
    ccvm_type_t* type,
    ccvm_basic_block_t** incoming_blocks,
    ccvm_value_t** incoming_values,
    size_t incoming_count
);

// Type creation functions
ccvm_type_t* ccvm_type_create_void(void);
ccvm_type_t* ccvm_type_create_integer(enum ccvm_value_type type, uint32_t bit_width);
ccvm_type_t* ccvm_type_create_float(enum ccvm_value_type type);
ccvm_type_t* ccvm_type_create_pointer(ccvm_type_t* element_type);
ccvm_type_t* ccvm_type_create_array(ccvm_type_t* element_type, size_t size);
ccvm_type_t* ccvm_type_create_vector(ccvm_type_t* element_type, size_t size);
ccvm_type_t* ccvm_type_create_function(
    ccvm_type_t* return_type,
    ccvm_type_t** param_types,
    size_t param_count,
    bool is_variadic
);
ccvm_type_t* ccvm_type_create_struct(
    const char* name,
    ccvm_type_t** field_types,
    size_t field_count,
    bool is_packed
);

// Constant creation functions
ccvm_value_t* ccvm_constant_create_integer(ccvm_type_t* type, int64_t value);
ccvm_value_t* ccvm_constant_create_float(ccvm_type_t* type, double value);
ccvm_value_t* ccvm_constant_create_null(ccvm_type_t* type);
ccvm_value_t* ccvm_constant_create_undef(ccvm_type_t* type);
ccvm_value_t* ccvm_constant_create_array(
    ccvm_type_t* type,
    ccvm_value_t** elements,
    size_t element_count
);
ccvm_value_t* ccvm_constant_create_struct(
    ccvm_type_t* type,
    ccvm_value_t** elements,
    size_t element_count
);

// Utility functions
const char* ccvm_opcode_to_string(enum ccvm_opcode opcode);
const char* ccvm_type_to_string(ccvm_type_t* type);
const char* ccvm_value_to_string(ccvm_value_t* value);
bool ccvm_type_is_integer(ccvm_type_t* type);
bool ccvm_type_is_float(ccvm_type_t* type);
bool ccvm_type_is_pointer(ccvm_type_t* type);
bool ccvm_type_is_array(ccvm_type_t* type);
bool ccvm_type_is_vector(ccvm_type_t* type);
bool ccvm_type_is_function(ccvm_type_t* type);
uint32_t ccvm_type_get_bit_width(ccvm_type_t* type);
size_t ccvm_type_get_size(ccvm_type_t* type);
size_t ccvm_type_get_alignment(ccvm_type_t* type);

// Instruction queries
bool ccvm_instruction_is_terminator(ccvm_instruction_t* inst);
bool ccvm_instruction_is_binary_op(ccvm_instruction_t* inst);
bool ccvm_instruction_is_memory_op(ccvm_instruction_t* inst);
bool ccvm_instruction_is_cast(ccvm_instruction_t* inst);
bool ccvm_instruction_is_commutative(ccvm_instruction_t* inst);
bool ccvm_instruction_may_have_side_effects(ccvm_instruction_t* inst);
bool ccvm_instruction_may_read_memory(ccvm_instruction_t* inst);
bool ccvm_instruction_may_write_memory(ccvm_instruction_t* inst);

// Value queries
bool ccvm_value_is_constant(ccvm_value_t* value);
bool ccvm_value_is_undef(ccvm_value_t* value);
bool ccvm_value_is_null(ccvm_value_t* value);
int64_t ccvm_constant_get_integer(ccvm_value_t* constant);
double ccvm_constant_get_float(ccvm_value_t* constant);
ccvm_type_t* ccvm_value_get_type(ccvm_value_t* value);

// Module queries
size_t ccvm_module_get_function_count(ccvm_module_t* module);
ccvm_function_t* ccvm_module_get_function(ccvm_module_t* module, size_t index);
ccvm_function_t* ccvm_module_get_function_by_name(ccvm_module_t* module, const char* name);
size_t ccvm_module_get_global_count(ccvm_module_t* module);
ccvm_value_t* ccvm_module_get_global(ccvm_module_t* module, size_t index);

// Function queries
size_t ccvm_function_get_basic_block_count(ccvm_function_t* function);
ccvm_basic_block_t* ccvm_function_get_basic_block(ccvm_function_t* function, size_t index);
ccvm_basic_block_t* ccvm_function_get_basic_block_by_name(ccvm_function_t* function, const char* name);
size_t ccvm_function_get_argument_count(ccvm_function_t* function);
ccvm_value_t* ccvm_function_get_argument(ccvm_function_t* function, size_t index);

// Basic block queries
size_t ccvm_basic_block_get_instruction_count(ccvm_basic_block_t* block);
ccvm_instruction_t* ccvm_basic_block_get_instruction(ccvm_basic_block_t* block, size_t index);
ccvm_instruction_t* ccvm_basic_block_get_terminator(ccvm_basic_block_t* block);
size_t ccvm_basic_block_get_predecessor_count(ccvm_basic_block_t* block);
ccvm_basic_block_t* ccvm_basic_block_get_predecessor(ccvm_basic_block_t* block, size_t index);
size_t ccvm_basic_block_get_successor_count(ccvm_basic_block_t* block);
ccvm_basic_block_t* ccvm_basic_block_get_successor(ccvm_basic_block_t* block, size_t index);

// IR verification
bool ccvm_module_verify(ccvm_module_t* module, char** error_message);
bool ccvm_function_verify(ccvm_function_t* function, char** error_message);
bool ccvm_basic_block_verify(ccvm_basic_block_t* block, char** error_message);
bool ccvm_instruction_verify(ccvm_instruction_t* instruction, char** error_message);

// IR printing
void ccvm_module_print(ccvm_module_t* module, FILE* file);
void ccvm_function_print(ccvm_function_t* function, FILE* file);
void ccvm_basic_block_print(ccvm_basic_block_t* block, FILE* file);
void ccvm_instruction_print(ccvm_instruction_t* instruction, FILE* file);
void ccvm_value_print(ccvm_value_t* value, FILE* file);

#ifdef __cplusplus
}
#endif

#endif // CCVM_OPTIMIZER_IR_TYPES_H