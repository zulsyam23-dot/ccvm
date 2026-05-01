/**
 * @file isel.c
 * @brief Instruction Selection for x86_64 - generates NASM assembly from CCVM IR
 * @author CCVM Team
 * @date 2026
 */

#include "ccvm/backend/ir.h"
#include "ccvm/backend/codegen.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

/* Register allocation for locals (temps) - we use a simple stack-based alloc */
typedef struct {
    char var_name[CCVM_IR_MAX_NAME];
    int offset;  /* relative to RBP */
} local_var_t;

typedef struct {
    local_var_t locals[512];
    int local_count;
    int stack_ptr;  /* current stack pointer offset from RBP */
} reg_alloc_t;

static int alloc_local(reg_alloc_t* ra, const char* name) {
    for (int i = 0; i < ra->local_count; i++) {
        if (strcmp(ra->locals[i].var_name, name) == 0) {
            return ra->locals[i].offset;
        }
    }
    int offset = -(ra->stack_ptr + 8);
    if (ra->local_count < 512) {
        strncpy(ra->locals[ra->local_count].var_name, name, CCVM_IR_MAX_NAME - 1);
        ra->locals[ra->local_count].offset = offset;
    }
    ra->local_count++;
    ra->stack_ptr += 8;
    return offset;
}

static void alloc_param(reg_alloc_t* ra, const char* name, int idx) {
    /* Win64: params at [rbp+16], [rbp+24], ... */
    int offset = 16 + idx * 8;
    if (ra->local_count < 512) {
        strncpy(ra->locals[ra->local_count].var_name, name, CCVM_IR_MAX_NAME - 1);
        ra->locals[ra->local_count].offset = offset;
    }
    ra->local_count++;
}

static int find_local(reg_alloc_t* ra, const char* name) {
    for (int i = 0; i < ra->local_count; i++) {
        if (strcmp(ra->locals[i].var_name, name) == 0) {
            return ra->locals[i].offset;
        }
    }
    return 0;
}

static void emit(FILE* out, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    /* Process format string to fix [rbp+-N] -> [rbp-N] */
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    /* Replace [rbp+- with [rbp- (remove the extra +) */
    char* p = strstr(buffer, "[rbp+-");
    if (p) {
        memmove(p + 4, p + 5, strlen(p + 5) + 1);
    }
    fprintf(out, "%s\n", buffer);
}

/* Parse operand from IR text representation */
static ccvm_ir_operand_t parse_operand(const char* str) {
    ccvm_ir_operand_t op;
    memset(&op, 0, sizeof(op));

    if (strncmp(str, "%", 1) == 0) {
        /* Named register/temp */
        if (strncmp(str, "%t", 2) == 0 || strncmp(str, "%%", 2) == 0) {
            op.kind = CCVM_IR_OP_REG;
            strncpy(op.mem, str, CCVM_IR_MAX_NAME - 1);
        } else {
            op.kind = CCVM_IR_OP_MEM;
            strncpy(op.mem, str, CCVM_IR_MAX_NAME - 1);
        }
    } else if (str[0] >= '0' && str[0] <= '9') {
        op.kind = CCVM_IR_OP_IMM;
        op.imm = atoll(str);
    } else {
        op.kind = CCVM_IR_OP_IMM;
        op.imm = 0;
    }
    return op;
}

/* Get x86_64 register for a CCVM virtual register (simple: use RAX, R8-R15 as scratch) */
static const char* scratch_reg(int idx) {
    static const char* regs[] = {"rax", "r8", "r9", "r10", "r11", "rcx", "rdx", "rsi", "rdi"};
    if (idx >= 0 && idx < 9) return regs[idx];
    return "rax";
}

/* MASM reserved words that conflict with function names */
static const char* masm_reserved[] = {
    "add", "sub", "mul", "div", "and", "or", "xor", "not", "neg",
    "inc", "dec", "shl", "shr", "sar", "rol", "ror", "rcl", "rcr",
    "test", "cmp", "mov", "lea", "push", "pop", "call", "ret",
    "jmp", "je", "jne", "jg", "jge", "jl", "jle", "ja", "jae", "jb", "jbe",
    "sete", "setne", "setg", "setge", "setl", "setle",
    "loop", "enter", "leave", "nop", "int", "iret",
    NULL
};

static int is_masm_reserved(const char* name) {
    for (int i = 0; masm_reserved[i]; i++) {
        if (strcmp(masm_reserved[i], name) == 0) return 1;
    }
    return 0;
}

static const char* safe_name(const char* name) {
    static char buf[256];
    if (is_masm_reserved(name)) {
        snprintf(buf, sizeof(buf), "_%s", name);
        return buf;
    }
    return name;
}

/* Translate a single IR function to x86_64 assembly */
static void translate_function(FILE* out, const ccvm_ir_func_t* func, reg_alloc_t* ra) {
    const char* fname = safe_name(func->name);
    ra->local_count = 0;
    ra->stack_ptr = 0;

    /* Prologue */
    emit(out, "");
    emit(out, "%s PROC", fname);
    emit(out, "    push rbp");
    emit(out, "    mov rbp, rsp");

    /* First pass: allocate all stack slots (params, allocas, and temps) */
    /* Register params (0-3) at [rbp+16], [rbp+24], [rbp+32], [rbp+40] */
    /* Stack params (4+) at [rbp+48], [rbp+56], [rbp+64], [rbp+72] */
    static const char* param_regs[] = {"rcx", "rdx", "r8", "r9"};
    for (int p = 0; p < func->param_count && p < 4; p++) {
        alloc_param(ra, func->params[p].name, p);
    }
    for (int p = 4; p < func->param_count; p++) {
        int offset = 48 + (p - 4) * 8;
        if (ra->local_count < 512) {
            strncpy(ra->locals[ra->local_count].var_name, func->params[p].name, CCVM_IR_MAX_NAME - 1);
            ra->locals[ra->local_count].offset = offset;
        }
        ra->local_count++;
    }

    /* Register alloca variables and pre-allocate temps from all instructions */
    for (int bi = 0; bi < func->block_count; bi++) {
        const ccvm_ir_block_t* block = &func->blocks[bi];
        for (int ii = 0; ii < block->instr_count; ii++) {
            const ccvm_ir_instr_t* instr = &block->instrs[ii];
            if (instr->opcode == CCVM_IR_ALLOCA && instr->result[0] != '\0') {
                alloc_local(ra, instr->result);
            }
            if (instr->result[0] != '\0' && instr->opcode != CCVM_IR_ALLOCA) {
                alloc_local(ra, instr->result);
            }
        }
    }

    /* Emit param saves */
    for (int p = 0; p < func->param_count && p < 4; p++) {
        int off = find_local(ra, func->params[p].name);
        emit(out, "    mov [rbp+%d], %s", off, param_regs[p]);
    }

    /* Reserve stack for negative-offset locals */
    int stack_size = 0;
    for (int i = 0; i < ra->local_count; i++) {
        if (ra->locals[i].offset < 0) {
            int abs_off = -ra->locals[i].offset;
            if (abs_off > stack_size) stack_size = abs_off;
        }
    }

    if (stack_size > 0) {
        emit(out, "    sub rsp, %d", stack_size);
    }

    /* Emit all blocks */
    for (int bi = 0; bi < func->block_count; bi++) {
        const ccvm_ir_block_t* block = &func->blocks[bi];
        if (strcmp(block->name, "entry") != 0) {
            /* Strip % if present */
            const char* label_name = block->name;
            if (label_name[0] == '%') label_name++;
            emit(out, "%s:", label_name);
        }

        for (int ii = 0; ii < block->instr_count; ii++) {
            const ccvm_ir_instr_t* instr = &block->instrs[ii];

            switch (instr->opcode) {
                case CCVM_IR_ADD: {
                    int off1 = alloc_local(ra, instr->op1.mem);
                    int off_res = alloc_local(ra, instr->result);
                    emit(out, "    mov rax, [rbp+%d]", off1);
                    if (instr->op2.kind == CCVM_IR_OP_IMM) {
                        emit(out, "    add rax, %lld", instr->op2.imm);
                    } else {
                        int off2 = alloc_local(ra, instr->op2.mem);
                        emit(out, "    add rax, [rbp+%d]", off2);
                    }
                    emit(out, "    mov [rbp+%d], rax", off_res);
                    break;
                }
                case CCVM_IR_SUB: {
                    int off1 = alloc_local(ra, instr->op1.mem);
                    int off_res = alloc_local(ra, instr->result);
                    emit(out, "    mov rax, [rbp+%d]", off1);
                    if (instr->op2.kind == CCVM_IR_OP_IMM) {
                        emit(out, "    sub rax, %lld", instr->op2.imm);
                    } else {
                        int off2 = alloc_local(ra, instr->op2.mem);
                        emit(out, "    sub rax, [rbp+%d]", off2);
                    }
                    emit(out, "    mov [rbp+%d], rax", off_res);
                    break;
                }
                case CCVM_IR_MUL: {
                    int off1 = alloc_local(ra, instr->op1.mem);
                    int off_res = alloc_local(ra, instr->result);
                    emit(out, "    mov rax, [rbp+%d]", off1);
                    if (instr->op2.kind == CCVM_IR_OP_IMM) {
                        emit(out, "    imul rax, %lld", instr->op2.imm);
                    } else {
                        int off2 = alloc_local(ra, instr->op2.mem);
                        emit(out, "    imul rax, [rbp+%d]", off2);
                    }
                    emit(out, "    mov [rbp+%d], rax", off_res);
                    break;
                }
                case CCVM_IR_SDIV: {
                    int off1 = alloc_local(ra, instr->op1.mem);
                    int off_res = alloc_local(ra, instr->result);
                    emit(out, "    mov rax, [rbp+%d]", off1);
                    emit(out, "    cqo");
                    if (instr->op2.kind == CCVM_IR_OP_IMM) {
                        emit(out, "    mov rbx, %lld", instr->op2.imm);
                    } else {
                        int off2 = alloc_local(ra, instr->op2.mem);
                        emit(out, "    mov rbx, [rbp+%d]", off2);
                    }
                    emit(out, "    idiv rbx");
                    emit(out, "    mov [rbp+%d], rax", off_res);
                    break;
                }
                case CCVM_IR_AND: {
                    int off1 = alloc_local(ra, instr->op1.mem);
                    int off_res = alloc_local(ra, instr->result);
                    emit(out, "    mov rax, [rbp+%d]", off1);
                    if (instr->op2.kind == CCVM_IR_OP_IMM) {
                        emit(out, "    and rax, %lld", instr->op2.imm);
                    } else {
                        int off2 = alloc_local(ra, instr->op2.mem);
                        emit(out, "    and rax, [rbp+%d]", off2);
                    }
                    emit(out, "    mov [rbp+%d], rax", off_res);
                    break;
                }
                case CCVM_IR_OR: {
                    int off1 = alloc_local(ra, instr->op1.mem);
                    int off_res = alloc_local(ra, instr->result);
                    emit(out, "    mov rax, [rbp+%d]", off1);
                    if (instr->op2.kind == CCVM_IR_OP_IMM) {
                        emit(out, "    or rax, %lld", instr->op2.imm);
                    } else {
                        int off2 = alloc_local(ra, instr->op2.mem);
                        emit(out, "    or rax, [rbp+%d]", off2);
                    }
                    emit(out, "    mov [rbp+%d], rax", off_res);
                    break;
                }
                case CCVM_IR_XOR: {
                    int off1 = alloc_local(ra, instr->op1.mem);
                    int off_res = alloc_local(ra, instr->result);
                    emit(out, "    mov rax, [rbp+%d]", off1);
                    if (instr->op2.kind == CCVM_IR_OP_IMM) {
                        emit(out, "    xor rax, %lld", instr->op2.imm);
                    } else {
                        int off2 = alloc_local(ra, instr->op2.mem);
                        emit(out, "    xor rax, [rbp+%d]", off2);
                    }
                    emit(out, "    mov [rbp+%d]", off_res);
                    break;
                }
                case CCVM_IR_ICMP_EQ:
                case CCVM_IR_ICMP_NE:
                case CCVM_IR_ICMP_SGT:
                case CCVM_IR_ICMP_SGE:
                case CCVM_IR_ICMP_SLT:
                case CCVM_IR_ICMP_SLE: {
                    int off1 = alloc_local(ra, instr->op1.mem);
                    int off_res = alloc_local(ra, instr->result);
                    const char* jcc = "sete";
                    switch (instr->opcode) {
                        case CCVM_IR_ICMP_EQ: jcc = "sete"; break;
                        case CCVM_IR_ICMP_NE: jcc = "setne"; break;
                        case CCVM_IR_ICMP_SGT: jcc = "setg"; break;
                        case CCVM_IR_ICMP_SGE: jcc = "setge"; break;
                        case CCVM_IR_ICMP_SLT: jcc = "setl"; break;
                        case CCVM_IR_ICMP_SLE: jcc = "setle"; break;
                        default: break;
                    }
                    emit(out, "    xor rax, rax");
                    emit(out, "    mov rax, [rbp+%d]", off1);
                    if (instr->op2.kind == CCVM_IR_OP_IMM) {
                        emit(out, "    cmp rax, %lld", instr->op2.imm);
                    } else {
                        int off2 = alloc_local(ra, instr->op2.mem);
                        emit(out, "    cmp rax, [rbp+%d]", off2);
                    }
                    emit(out, "    %s al", jcc);
                    emit(out, "    mov [rbp+%d], rax", off_res);
                    break;
                }
                case CCVM_IR_ALLOCA: {
                    /* Already handled in prologue */
                    break;
                }
                case CCVM_IR_LOAD: {
                    /* Load value from stack slot (alloca variable is the slot itself) */
                    int off_addr = alloc_local(ra, instr->op1.mem);
                    int off_res = alloc_local(ra, instr->result);
                    emit(out, "    mov rax, [rbp+%d]", off_addr);
                    emit(out, "    mov [rbp+%d], rax", off_res);
                    break;
                }
                case CCVM_IR_STORE: {
                    /* Store to stack slot */
                    int off_val, off_addr;
                    if (instr->op1.kind == CCVM_IR_OP_IMM) {
                        emit(out, "    mov rax, %lld", instr->op1.imm);
                    } else {
                        off_val = alloc_local(ra, instr->op1.mem);
                        emit(out, "    mov rax, [rbp+%d]", off_val);
                    }
                    off_addr = alloc_local(ra, instr->op2.mem);
                    emit(out, "    mov [rbp+%d], rax", off_addr);
                    break;
                }
                case CCVM_IR_BR: {
                    const char* target = instr->op1.label;
                    if (target[0] == '%') target++;
                    emit(out, "    jmp %s", target);
                    break;
                }
                case CCVM_IR_COND_BR: {
                    int off_cond = alloc_local(ra, instr->op1.mem);
                    const char* then_label = instr->op2.label;
                    const char* else_label = instr->func_name[0] ? instr->func_name : (bi + 1 < func->block_count ? func->blocks[bi + 1].name : ".Lend");
                    if (then_label[0] == '%') then_label++;
                    if (else_label[0] == '%') else_label++;
                    emit(out, "    cmp byte ptr [rbp+%d], 0", off_cond);
                    emit(out, "    jne %s", then_label);
                    emit(out, "    jmp %s", else_label);
                    break;
                }
                case CCVM_IR_RET: {
                    int is_main = (strcmp(func->name, "main") == 0);
                    if (instr->op1.kind == CCVM_IR_OP_IMM) {
                        if (is_main) {
                            emit(out, "    mov rcx, %lld", instr->op1.imm);
                        } else {
                            emit(out, "    mov rax, %lld", instr->op1.imm);
                        }
                    } else {
                        int off = alloc_local(ra, instr->op1.mem);
                        if (is_main) {
                            emit(out, "    mov rcx, [rbp+%d]", off);
                        } else {
                            emit(out, "    mov rax, [rbp+%d]", off);
                        }
                    }
                    emit(out, "    mov rsp, rbp");
                    emit(out, "    pop rbp");
                    if (is_main) {
                        emit(out, "    and rsp, -16");
                        emit(out, "    call ExitProcess");
                    } else {
                        emit(out, "    ret");
                    }
                    break;
                }
                case CCVM_IR_CALL: {
                    /* Windows x64 calling convention: first 4 args in rcx, rdx, r8, r9 */
                    /* Args 5+ on stack. Shadow space (32 bytes) always reserved. */
                    static const char* arg_regs[] = {"rcx", "rdx", "r8", "r9"};
                    int stack_args = instr->arg_count > 4 ? instr->arg_count - 4 : 0;
                    int shadow = 32;
                    int total_stack = shadow + stack_args * 8;
                    
                    if (total_stack > 0) {
                        emit(out, "    sub rsp, %d", total_stack);
                    }
                    
                    for (int a = 0; a < instr->arg_count; a++) {
                        const ccvm_ir_operand_t* arg = &instr->args[a];
                        if (a < 4) {
                            if (arg->kind == CCVM_IR_OP_IMM) {
                                emit(out, "    mov %s, %lld", arg_regs[a], arg->imm);
                            } else {
                                int off = alloc_local(ra, arg->mem);
                                emit(out, "    mov %s, [rbp+%d]", arg_regs[a], off);
                            }
                        } else {
                            int stack_off = 32 + (a - 4) * 8;
                            if (arg->kind == CCVM_IR_OP_IMM) {
                                emit(out, "    mov rax, %lld", arg->imm);
                                emit(out, "    mov [rsp+%d], rax", stack_off);
                            } else {
                                int off = alloc_local(ra, arg->mem);
                                emit(out, "    mov rax, [rbp+%d]", off);
                                emit(out, "    mov [rsp+%d], rax", stack_off);
                            }
                        }
                    }
                    emit(out, "    call %s", safe_name(instr->func_name));
                    if (total_stack > 0) {
                        emit(out, "    add rsp, %d", total_stack);
                    }
                    if (instr->result[0] != '\0') {
                        int off_res = alloc_local(ra, instr->result);
                        emit(out, "    mov [rbp+%d], rax", off_res);
                    }
                    break;
                }
                case CCVM_IR_GEP: {
                    /* Load address of string constant */
                    const char* sname = instr->op1.mem;
                    if (sname[0] == '.') sname++;
                    int off_res = alloc_local(ra, instr->result);
                    emit(out, "    lea rax, %s", sname);
                    emit(out, "    mov [rbp+%d], rax", off_res);
                    break;
                }
                case CCVM_IR_LABEL:
                    /* Already emitted above */
                    break;
                default:
                    emit(out, "    ; unsupported: opcode %d", instr->opcode);
                    break;
            }
        }
    }

    /* Epilogue - only if last instruction wasn't a ret */
    if (func->block_count > 0) {
        ccvm_ir_block_t* last_block = &func->blocks[func->block_count - 1];
        int has_ret = 0;
        for (int ii = last_block->instr_count - 1; ii >= 0; ii--) {
            if (last_block->instrs[ii].opcode == CCVM_IR_RET) {
                has_ret = 1;
                break;
            }
        }
        if (!has_ret) {
            emit(out, "    mov rsp, rbp");
            emit(out, "    pop rbp");
            emit(out, "    ret");
        }
    }
    emit(out, "%s ENDP", fname);
}

static void emit_end(FILE* out) {
    fprintf(out, "\nEND\n");
}

/* Entry point for x86_64 backend */
int ccvm_x86_64_generate(const ccvm_ir_module_t* module, const char* output_file) {
    if (!module || !output_file) return 0;

    FILE* out = fopen(output_file, "w");
    if (!out) {
        fprintf(stderr, "Error: Cannot open output file: %s\n", output_file);
        return 0;
    }

    reg_alloc_t ra;
    memset(&ra, 0, sizeof(ra));

    /* Header */
    fprintf(out, "; CCVM x86_64 Assembly Output\n");
    fprintf(out, "; Generated from: %s\n\n", module->module_name);

    /* Data section for string constants */
    if (module->string_count > 0) {
        fprintf(out, ".data\n\n");
        for (int si = 0; si < module->string_count; si++) {
            const ccvm_ir_string_t* str = &module->strings[si];
            const char* sname = str->name;
            if (sname[0] == '.') sname++;
            fprintf(out, "%s db ", sname);
            /* Process escape sequences and write bytes */
            int first = 1;
            const char* p = str->value;
            while (*p) {
                unsigned char ch;
                if (*p == '\\' && p[1]) {
                    p++;
                    switch (*p) {
                        case 'n': ch = '\n'; break;
                        case 't': ch = '\t'; break;
                        case 'r': ch = '\r'; break;
                        case '0': ch = 0; while (p[1] >= '0' && p[1] <= '9') p++; break;
                        case '\\': ch = '\\'; break;
                        case '"': ch = '"'; break;
                        default: ch = *p; break;
                    }
                } else {
                    ch = (unsigned char)*p;
                }
                if (!first) fprintf(out, ", ");
                if (ch == 0) {
                    fprintf(out, "0");
                } else if (ch >= 32 && ch < 127) {
                    fprintf(out, "'%c'", ch);
                } else {
                    fprintf(out, "%d", ch);
                }
                first = 0;
                p++;
            }
            fprintf(out, "\n");
        }
        fprintf(out, "\n");
    }

    fprintf(out, ".code\n\n");

    /* External declarations - deduplicated, exclude functions defined in this module */
    int has_main = 0;
    for (int fi = 0; fi < module->func_count; fi++) {
        if (strcmp(module->funcs[fi].name, "main") == 0) { has_main = 1; break; }
    }
    if (has_main) {
        fprintf(out, "extrn ExitProcess: PROC\n");
    }
    char seen_funcs[CCVM_IR_MAX_FUNCS][CCVM_IR_MAX_NAME];
    int seen_count = 0;
    for (int fi = 0; fi < module->func_count; fi++) {
        const ccvm_ir_func_t* func = &module->funcs[fi];
        for (int bi = 0; bi < func->block_count; bi++) {
            const ccvm_ir_block_t* block = &func->blocks[bi];
            for (int ii = 0; ii < block->instr_count; ii++) {
                if (block->instrs[ii].opcode == CCVM_IR_CALL) {
                    const char* fname = block->instrs[ii].func_name;
                    /* Skip if this function is defined in the module */
                    int is_defined = 0;
                    for (int df = 0; df < module->func_count; df++) {
                        if (strcmp(module->funcs[df].name, fname) == 0) { is_defined = 1; break; }
                    }
                    if (is_defined) continue;
                    int found = 0;
                    for (int s = 0; s < seen_count; s++) {
                        if (strcmp(seen_funcs[s], fname) == 0) { found = 1; break; }
                    }
                    if (!found) {
                        fprintf(out, "extrn %s: PROC\n", safe_name(fname));
                        if (seen_count < CCVM_IR_MAX_FUNCS) {
                            strncpy(seen_funcs[seen_count++], fname, CCVM_IR_MAX_NAME - 1);
                        }
                    }
                }
            }
        }
    }
    if (module->func_count > 0) fprintf(out, "\n");

    /* Translate each function */
    for (int fi = 0; fi < module->func_count; fi++) {
        translate_function(out, &module->funcs[fi], &ra);
    }

    if (module->func_count > 0) {
        emit_end(out);
    }

    fclose(out);
    return 1;
}
