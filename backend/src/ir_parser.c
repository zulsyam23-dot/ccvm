/**
 * @file ir_parser.c
 * @brief Parse CCVM textual IR into C structures
 * @author CCVM Team
 * @date 2026
 */

#include "ccvm/backend/ir.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static char* trim(char* s) {
    while (isspace((unsigned char)*s)) s++;
    if (*s == '\0') return s;
    char* end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return s;
}

static ccvm_ir_opcode_t parse_opcode(const char* s) {
    if (strcmp(s, "add") == 0) return CCVM_IR_ADD;
    if (strcmp(s, "sub") == 0) return CCVM_IR_SUB;
    if (strcmp(s, "mul") == 0) return CCVM_IR_MUL;
    if (strcmp(s, "sdiv") == 0) return CCVM_IR_SDIV;
    if (strcmp(s, "and") == 0) return CCVM_IR_AND;
    if (strcmp(s, "or") == 0) return CCVM_IR_OR;
    if (strcmp(s, "xor") == 0) return CCVM_IR_XOR;
    if (strcmp(s, "shl") == 0) return CCVM_IR_SHL;
    if (strcmp(s, "ashr") == 0) return CCVM_IR_ASHR;
    if (strcmp(s, "lshr") == 0) return CCVM_IR_LSHR;
    if (strcmp(s, "icmp eq") == 0) return CCVM_IR_ICMP_EQ;
    if (strcmp(s, "icmp ne") == 0) return CCVM_IR_ICMP_NE;
    if (strcmp(s, "icmp sgt") == 0) return CCVM_IR_ICMP_SGT;
    if (strcmp(s, "icmp sge") == 0) return CCVM_IR_ICMP_SGE;
    if (strcmp(s, "icmp slt") == 0) return CCVM_IR_ICMP_SLT;
    if (strcmp(s, "icmp sle") == 0) return CCVM_IR_ICMP_SLE;
    if (strcmp(s, "alloca") == 0) return CCVM_IR_ALLOCA;
    if (strcmp(s, "load") == 0) return CCVM_IR_LOAD;
    if (strcmp(s, "store") == 0) return CCVM_IR_STORE;
    if (strcmp(s, "call") == 0) return CCVM_IR_CALL;
    if (strcmp(s, "br") == 0) return CCVM_IR_BR;
    if (strcmp(s, "ret") == 0) return CCVM_IR_RET;
    return CCVM_IR_NOP;
}

static ccvm_ir_type_t parse_type(const char* s) {
    if (strcmp(s, "void") == 0) return CCVM_IR_VOID;
    if (strcmp(s, "i1") == 0) return CCVM_IR_I1;
    if (strcmp(s, "i8") == 0) return CCVM_IR_I8;
    if (strcmp(s, "i16") == 0) return CCVM_IR_I16;
    if (strcmp(s, "i32") == 0) return CCVM_IR_I32;
    if (strcmp(s, "i64") == 0) return CCVM_IR_I64;
    if (strcmp(s, "f32") == 0) return CCVM_IR_F32;
    if (strcmp(s, "f64") == 0) return CCVM_IR_F64;
    if (strcmp(s, "ptr") == 0) return CCVM_IR_PTR;
    return CCVM_IR_I64;
}

static ccvm_ir_operand_t parse_operand_str(const char* s) {
    ccvm_ir_operand_t op;
    memset(&op, 0, sizeof(op));

    if (strncmp(s, "%", 1) == 0) {
        /* Strip the % prefix for consistent naming */
        s++;
        strncpy(op.mem, s, CCVM_IR_MAX_NAME - 1);
        op.kind = CCVM_IR_OP_MEM;
    } else if (isdigit((unsigned char)s[0]) || (s[0] == '-' && isdigit((unsigned char)s[1]))) {
        op.kind = CCVM_IR_OP_IMM;
        op.imm = atoll(s);
    } else {
        op.kind = CCVM_IR_OP_IMM;
        op.imm = 0;
    }
    return op;
}

static ccvm_ir_operand_t parse_label_str(const char* s) {
    ccvm_ir_operand_t op;
    memset(&op, 0, sizeof(op));
    op.kind = CCVM_IR_OP_LABEL;
    /* Strip % if present */
    if (strncmp(s, "%", 1) == 0) s++;
    strncpy(op.label, s, CCVM_IR_MAX_NAME - 1);
    return op;
}

int ccvm_ir_parse(const char* filename, ccvm_ir_module_t* module) {
    FILE* f = fopen(filename, "r");
    if (!f) return 0;

    memset(module, 0, sizeof(*module));
    char line[1024];

    ccvm_ir_func_t* current_func = NULL;
    ccvm_ir_block_t* current_block = NULL;

    while (fgets(line, sizeof(line), f)) {
        char* l = trim(line);
        if (l[0] == ';' || l[0] == '\0') continue;

        /* Skip closing brace */
        if (l[0] == '}') continue;

        /* Function declaration: func name(params) -> ret { */
        if (strncmp(l, "func ", 5) == 0) {
            if (module->func_count >= CCVM_IR_MAX_FUNCS) break;
            current_func = &module->funcs[module->func_count++];

            char* name_start = l + 5;
            char* brace = strchr(name_start, '{');
            if (brace) *brace = '\0';
            char* paren = strchr(name_start, '(');
            if (paren) {
                *paren = '\0';
                strncpy(current_func->name, trim(name_start), CCVM_IR_MAX_NAME - 1);

                /* Parse params */
                char* paren_end = strchr(paren + 1, ')');
                if (paren_end) {
                    *paren_end = '\0';
                    char* params = paren + 1;
                    if (strlen(trim(params)) > 0) {
                        char* tok = strtok(params, ",");
                        while (tok && current_func->param_count < CCVM_IR_MAX_PARAMS) {
                            tok = trim(tok);
                            char* space = strchr(tok, ' ');
                            if (space) {
                                *space = '\0';
                                char* pname = trim(space + 1);
                                if (pname[0] == '%') pname++;
                                strncpy(current_func->params[current_func->param_count].name, pname, CCVM_IR_MAX_NAME - 1);
                                current_func->params[current_func->param_count].type = parse_type(tok);
                                current_func->param_count++;
                            }
                            tok = strtok(NULL, ",");
                        }
                    }
                    /* Parse return type */
                    char* arrow = strstr(paren_end + 1, "->");
                    if (arrow) {
                        char* ret_str = trim(arrow + 2);
                        /* Remove trailing { */
                        char* brace = strchr(ret_str, '{');
                        if (brace) *brace = '\0';
                        current_func->return_type = parse_type(ret_str);
                    }
                }
            }
            current_block = NULL;
            continue;
        }

        /* Label: label_name: or %label_name: */
        if (l[0] != '\0' && l[strlen(l) - 1] == ':' && l[0] != '{') {
            if (current_func) {
                l[strlen(l) - 1] = '\0';
                if (current_func->block_count < CCVM_IR_MAX_BLOCKS) {
                    current_block = &current_func->blocks[current_func->block_count++];
                    strncpy(current_block->name, l, CCVM_IR_MAX_NAME - 1);
                    current_block->instr_count = 0;
                }
            }
            continue;
        }

        /* Instruction */
        if (current_block && current_block->instr_count < CCVM_IR_MAX_INSTRS) {
            ccvm_ir_instr_t* instr = &current_block->instrs[current_block->instr_count];
            memset(instr, 0, sizeof(*instr));

            char result_name[CCVM_IR_MAX_NAME] = {0};
            char opcode_str[64] = {0};

            /* Check for result = ... */
            char* eq = strchr(l, '=');
            const char* instr_start = l;
            if (eq && eq > l) {
                /* Look for % anywhere before = */
                char* pct = NULL;
                for (char* p = l; p < eq; p++) {
                    if (*p == '%') {
                        pct = p;
                    }
                }
                if (pct) {
                    /* Parse result name */
                    pct++; /* skip % */
                    char* res_end = pct;
                    while (*res_end && *res_end != ' ' && res_end < eq) res_end++;
                    int len = (int)(res_end - pct);
                    if (len > 0 && len < CCVM_IR_MAX_NAME) {
                        strncpy(result_name, pct, len);
                        result_name[len] = '\0';
                    }
                    instr_start = eq + 1;
                    while (*instr_start == ' ') instr_start++;
                }
            }

            /* Extract opcode */
            const char* space = strchr(instr_start, ' ');
            if (space) {
                int olen = (int)(space - instr_start);
                if (olen < 64) {
                    strncpy(opcode_str, instr_start, olen);
                    opcode_str[olen] = '\0';
                }
                instr->opcode = parse_opcode(opcode_str);

                /* Check for icmp */
                if (strcmp(opcode_str, "icmp") == 0) {
                    /* "icmp sle i64 %x, 1" */
                    const char* rest = space + 1;
                    while (*rest == ' ') rest++;
                    const char* next_space = strchr(rest, ' ');
                    if (next_space) {
                        int icmp_len = (int)(next_space - instr_start);
                        if (icmp_len < 64) {
                            strncpy(opcode_str, instr_start, icmp_len);
                            opcode_str[icmp_len] = '\0';
                            instr->opcode = parse_opcode(opcode_str);
                        }
                        instr_start = next_space;
                        while (*instr_start == ' ') instr_start++;
                    }
                }
            } else {
                instr->opcode = parse_opcode(l);
                if (instr->opcode == CCVM_IR_NOP) {
                    /* Could be a label */
                    if (l[strlen(l) - 1] == ':') {
                        l[strlen(l) - 1] = '\0';
                        strncpy(current_block->name, l, CCVM_IR_MAX_NAME - 1);
                        continue;
                    }
                }
            }

            strncpy(instr->result, result_name, CCVM_IR_MAX_NAME - 1);

            const char* args = instr_start;
            if (space) {
                args = space + 1;
                while (*args == ' ') args++;
            } else {
                args = l + strlen(opcode_str);
                while (*args == ' ') args++;
            }

            switch (instr->opcode) {
                case CCVM_IR_ADD:
                case CCVM_IR_SUB:
                case CCVM_IR_MUL:
                case CCVM_IR_SDIV:
                case CCVM_IR_AND:
                case CCVM_IR_OR:
                case CCVM_IR_XOR:
                case CCVM_IR_ICMP_EQ:
                case CCVM_IR_ICMP_NE:
                case CCVM_IR_ICMP_SGT:
                case CCVM_IR_ICMP_SGE:
                case CCVM_IR_ICMP_SLT:
                case CCVM_IR_ICMP_SLE: {
                    /* type %op1, %op2 */
                    char args_copy[512];
                    strncpy(args_copy, args, sizeof(args_copy) - 1);
                    char* comma = strchr(args_copy, ',');
                    if (comma) {
                        *comma = '\0';
                        /* Skip type */
                        char* op1_str = strchr(args_copy, '%');
                        if (op1_str) {
                            /* Find next comma or space after type */
                            char* after_type = strchr(op1_str, ' ');
                            if (after_type) op1_str = after_type + 1;
                            while (*op1_str == ' ') op1_str++;
                            instr->op1 = parse_operand_str(op1_str);
                        }
                        char* op2_str = comma + 1;
                        while (*op2_str == ' ') op2_str++;
                        instr->op2 = parse_operand_str(op2_str);
                    }
                    break;
                }
                case CCVM_IR_ALLOCA: {
                    /* i64 or similar */
                    break;
                }
                case CCVM_IR_STORE: {
                    /* i64 %val, ptr %addr  OR  i64 42, ptr %x */
                    char args_copy[512];
                    strncpy(args_copy, args, sizeof(args_copy) - 1);
                    char* comma = strchr(args_copy, ',');
                    if (comma) {
                        *comma = '\0';
                        /* First operand: skip type, get value (could be imm or %) */
                        char* first = trim(args_copy);
                        /* Skip the type (first word) */
                        char* space = strchr(first, ' ');
                        char* val_str = space ? trim(space + 1) : first;
                        if (val_str[0] == '%') {
                            instr->op1 = parse_operand_str(val_str);
                        } else {
                            instr->op1.kind = CCVM_IR_OP_IMM;
                            instr->op1.imm = atoll(val_str);
                        }
                        /* Second operand: skip "ptr", get address */
                        char* second = trim(comma + 1);
                        space = strchr(second, ' ');
                        char* addr_str = space ? trim(space + 1) : second;
                        if (addr_str[0] == '%') {
                            instr->op2 = parse_operand_str(addr_str);
                        }
                    }
                    break;
                }
                case CCVM_IR_LOAD: {
                    /* i64, ptr %addr */
                    char* ptr = strchr(args, '%');
                    if (ptr) {
                        instr->op1 = parse_operand_str(ptr);
                    }
                    break;
                }
                case CCVM_IR_RET: {
                    /* i64 %val, i64 42, or void */
                    if (strstr(args, "void")) {
                        instr->op1.kind = CCVM_IR_OP_IMM;
                        instr->op1.imm = 0;
                    } else {
                        /* Skip type prefix like "i64 " */
                        char* val_str = (char*)args;
                        char* sp = strchr(val_str, ' ');
                        if (sp) val_str = trim(sp + 1);
                        if (val_str[0] == '%') {
                            instr->op1 = parse_operand_str(val_str);
                        } else if (isdigit((unsigned char)*val_str) || (*val_str == '-' && isdigit((unsigned char)*(val_str+1)))) {
                            instr->op1.kind = CCVM_IR_OP_IMM;
                            instr->op1.imm = atoll(val_str);
                        }
                    }
                    break;
                }
                case CCVM_IR_BR: {
                    if (strstr(args, "i1")) {
                        /* Conditional: br i1 %cond, label %then, label %else */
                        char args_copy[512];
                        strncpy(args_copy, args, sizeof(args_copy) - 1);
                        /* Parse: i1 %cond, label %then, label %else */
                        char* comma1 = strchr(args_copy, ',');
                        if (comma1) {
                            *comma1 = '\0';
                            /* Get condition */
                            char* cond = strchr(args_copy, '%');
                            if (cond) instr->op1 = parse_operand_str(cond);
                            /* Get then label */
                            char* rest = comma1 + 1;
                            char* pct = strchr(rest, '%');
                            char* comma2 = strchr(rest, ',');
                            if (comma2) {
                                *comma2 = '\0';
                                if (pct) instr->op2 = parse_label_str(pct);
                                /* Get else label */
                                rest = comma2 + 1;
                                pct = strchr(rest, '%');
                                if (pct) {
                                    /* Store else label temporarily in func_name field */
                                    strncpy(instr->func_name, pct, CCVM_IR_MAX_NAME - 1);
                                }
                            }
                        }
                        instr->opcode = CCVM_IR_COND_BR;
                    } else {
                        /* Unconditional: br label %label */
                        char* label = strchr(args, '%');
                        if (label) {
                            strncpy(instr->op1.label, label, CCVM_IR_MAX_NAME - 1);
                        }
                    }
                    break;
                }
                case CCVM_IR_CALL: {
                    /* i64 @func(i64 %arg1, ...) */
                    char* at = strchr(args, '@');
                    if (at) {
                        at++;
                        char* paren = strchr(at, '(');
                        if (paren) {
                            char func_buf[CCVM_IR_MAX_NAME];
                            int flen = (int)(paren - at);
                            if (flen < CCVM_IR_MAX_NAME) {
                                strncpy(func_buf, at, flen);
                                func_buf[flen] = '\0';
                                strncpy(instr->func_name, func_buf, CCVM_IR_MAX_NAME - 1);
                            }

                            /* Parse args */
                            char* arg_start = paren + 1;
                            char* arg_end = strchr(arg_start, ')');
                            if (arg_end) {
                                char args_buf[512];
                                int alen = (int)(arg_end - arg_start);
                                if (alen < 512) {
                                    strncpy(args_buf, arg_start, alen);
                                    args_buf[alen] = '\0';

                                    if (strlen(trim(args_buf)) > 0) {
                                        char* tok = strtok(args_buf, ",");
                                        while (tok && instr->arg_count < CCVM_IR_MAX_PARAMS) {
                                            tok = trim(tok);
                                            /* Skip type prefix like "i64 " */
                                            char* space = strchr(tok, ' ');
                                            if (space) tok = trim(space + 1);
                                            char* pct = strchr(tok, '%');
                                            if (pct) {
                                                instr->args[instr->arg_count] = parse_operand_str(pct);
                                            } else if (isdigit((unsigned char)*tok) || (*tok == '-' && isdigit((unsigned char)*(tok+1)))) {
                                                instr->args[instr->arg_count].kind = CCVM_IR_OP_IMM;
                                                instr->args[instr->arg_count].imm = atoll(tok);
                                            }
                                            instr->arg_count++;
                                            tok = strtok(NULL, ",");
                                        }
                                    }
                                }
                            }
                        }
                    }
                    break;
                }
                default:
                    break;
            }

            current_block->instr_count++;
        }
    }

    fclose(f);
    return 1;
}
