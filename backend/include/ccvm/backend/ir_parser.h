/**
 * @file ir_parser.h
 * @brief Parse CCVM textual IR into C structures
 * @author CCVM Team
 * @date 2026
 */

#ifndef CCVM_BACKEND_IR_PARSER_H
#define CCVM_BACKEND_IR_PARSER_H

#include "ccvm/backend/ir.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Parse textual IR file into a ccvm_ir_module_t
 * @param filename Path to .ir file
 * @param module Output module structure
 * @return 1 on success, 0 on failure
 */
int ccvm_ir_parse(const char* filename, ccvm_ir_module_t* module);

#ifdef __cplusplus
}
#endif

#endif
