/**
 * @file ir.h
 * @brief C-compatible IR structures for CCVM backend
 * @author CCVM Team
 * @date 2026
 */

#ifndef CCVM_BACKEND_IR_H
#define CCVM_BACKEND_IR_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CCVM_IR_MAX_PARAMS 4
#define CCVM_IR_MAX_BLOCKS 8
#define CCVM_IR_MAX_INSTRS 64
#define CCVM_IR_MAX_NAME 32
#define CCVM_IR_MAX_FUNCS 8

typedef enum {
    CCVM_IR_NOP,
    CCVM_IR_ADD,
    CCVM_IR_SUB,
    CCVM_IR_MUL,
    CCVM_IR_SDIV,
    CCVM_IR_SREM,
    CCVM_IR_AND,
    CCVM_IR_OR,
    CCVM_IR_XOR,
    CCVM_IR_SHL,
    CCVM_IR_ASHR,
    CCVM_IR_LSHR,
    CCVM_IR_ICMP_EQ,
    CCVM_IR_ICMP_NE,
    CCVM_IR_ICMP_SGT,
    CCVM_IR_ICMP_SGE,
    CCVM_IR_ICMP_SLT,
    CCVM_IR_ICMP_SLE,
    CCVM_IR_ALLOCA,
    CCVM_IR_LOAD,
    CCVM_IR_STORE,
    CCVM_IR_CALL,
    CCVM_IR_BR,
    CCVM_IR_COND_BR,
    CCVM_IR_RET,
    CCVM_IR_PHI,
    CCVM_IR_LABEL,
    CCVM_IR_SEXT,
    CCVM_IR_ZEXT,
    CCVM_IR_TRUNC
} ccvm_ir_opcode_t;

typedef enum {
    CCVM_IR_VOID,
    CCVM_IR_I1,
    CCVM_IR_I8,
    CCVM_IR_I16,
    CCVM_IR_I32,
    CCVM_IR_I64,
    CCVM_IR_F32,
    CCVM_IR_F64,
    CCVM_IR_PTR
} ccvm_ir_type_t;

typedef enum {
    CCVM_IR_OP_IMM,
    CCVM_IR_OP_REG,
    CCVM_IR_OP_MEM,
    CCVM_IR_OP_LABEL
} ccvm_ir_operand_kind_t;

typedef struct {
    ccvm_ir_operand_kind_t kind;
    union {
        int64_t imm;
        int reg;
        char mem[CCVM_IR_MAX_NAME];
        char label[CCVM_IR_MAX_NAME];
    };
} ccvm_ir_operand_t;

typedef struct {
    ccvm_ir_opcode_t opcode;
    ccvm_ir_type_t type;
    char result[CCVM_IR_MAX_NAME];
    ccvm_ir_operand_t op1;
    ccvm_ir_operand_t op2;
    char func_name[CCVM_IR_MAX_NAME];
    ccvm_ir_operand_t args[CCVM_IR_MAX_PARAMS];
    int arg_count;
} ccvm_ir_instr_t;

typedef struct {
    char name[CCVM_IR_MAX_NAME];
    ccvm_ir_instr_t instrs[CCVM_IR_MAX_INSTRS];
    int instr_count;
} ccvm_ir_block_t;

typedef struct {
    char name[CCVM_IR_MAX_NAME];
    ccvm_ir_type_t return_type;
    struct {
        char name[CCVM_IR_MAX_NAME];
        ccvm_ir_type_t type;
    } params[CCVM_IR_MAX_PARAMS];
    int param_count;
    ccvm_ir_block_t blocks[CCVM_IR_MAX_BLOCKS];
    int block_count;
} ccvm_ir_func_t;

typedef struct {
    char module_name[CCVM_IR_MAX_NAME];
    ccvm_ir_func_t funcs[CCVM_IR_MAX_FUNCS];
    int func_count;
} ccvm_ir_module_t;

#ifdef __cplusplus
}
#endif

#endif
