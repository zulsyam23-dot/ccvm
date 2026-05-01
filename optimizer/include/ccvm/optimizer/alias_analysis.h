#ifndef CCVM_OPTIMIZER_ALIAS_ANALYSIS_H
#define CCVM_OPTIMIZER_ALIAS_ANALYSIS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ir_types.h"
#include <stdbool.h>
#include <stddef.h>

// Alias analysis result
typedef enum ccvm_alias_result {
    CCVM_ALIAS_NO_ALIAS = 0,
    CCVM_ALIAS_MAY_ALIAS = 1,
    CCVM_ALIAS_MUST_ALIAS = 2,
    CCVM_ALIAS_PARTIAL_ALIAS = 3,
} ccvm_alias_result_t;

// Memory location
typedef struct ccvm_memory_location {
    ccvm_value_t* pointer;
    size_t offset;
    size_t size;
    bool is_volatile;
    bool is_constant;
} ccvm_memory_location_t;

// Alias analysis interface
typedef struct ccvm_alias_analysis {
    ccvm_function_t* function;
    void* private_data;
} ccvm_alias_analysis_t;

// Create alias analysis
ccvm_alias_analysis_t* ccvm_alias_analysis_create(ccvm_function_t* function);
void ccvm_alias_analysis_destroy(ccvm_alias_analysis_t* analysis);

// Alias queries
ccvm_alias_result_t ccvm_alias_analysis_query(
    ccvm_alias_analysis_t* analysis,
    ccvm_memory_location_t* loc1,
    ccvm_memory_location_t* loc2
);

ccvm_alias_result_t ccvm_alias_analysis_query_pointers(
    ccvm_alias_analysis_t* analysis,
    ccvm_value_t* ptr1,
    ccvm_value_t* ptr2
);

// Mod/ref analysis
typedef enum ccvm_mod_ref_result {
    CCVM_MOD_REF_NO_MOD_REF = 0,
    CCVM_MOD_REF_MAY_MOD = 1,
    CCVM_MOD_REF_MAY_REF = 2,
    CCVM_MOD_REF_MUST_MOD = 4,
    CCVM_MOD_REF_MUST_REF = 8,
} ccvm_mod_ref_result_t;

ccvm_mod_ref_result_t ccvm_alias_analysis_get_mod_ref_info(
    ccvm_alias_analysis_t* analysis,
    ccvm_instruction_t* instruction,
    ccvm_memory_location_t* location
);

// Points-to analysis
typedef struct ccvm_points_to_set {
    ccvm_value_t** targets;
    size_t target_count;
    bool is_complete;
} ccvm_points_to_set_t;

ccvm_points_to_set_t* ccvm_alias_analysis_get_points_to_set(
    ccvm_alias_analysis_t* analysis,
    ccvm_value_t* pointer
);

void ccvm_points_to_set_destroy(ccvm_points_to_set_t* set);

// Escape analysis
typedef enum ccvm_escape_result {
    CCVM_ESCAPE_NO_ESCAPE = 0,
    CCVM_ESCAPE_ARG_ESCAPE = 1,
    CCVM_ESCAPE_RETURN_ESCAPE = 2,
    CCVM_ESCAPE_GLOBAL_ESCAPE = 3,
} ccvm_escape_result_t;

ccvm_escape_result_t ccvm_alias_analysis_get_escape_result(
    ccvm_alias_analysis_t* analysis,
    ccvm_value_t* value
);

// Utility functions
void ccvm_alias_analysis_print(ccvm_alias_analysis_t* analysis, FILE* file);
const char* ccvm_alias_result_to_string(ccvm_alias_result_t result);
const char* ccvm_mod_ref_result_to_string(ccvm_mod_ref_result_t result);
const char* ccvm_escape_result_to_string(ccvm_escape_result_t result);

#ifdef __cplusplus
}
#endif

#endif // CCVM_OPTIMIZER_ALIAS_ANALYSIS_H