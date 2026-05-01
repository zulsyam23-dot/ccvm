#ifndef CCVM_OPTIMIZER_LOOP_ANALYSIS_H
#define CCVM_OPTIMIZER_LOOP_ANALYSIS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ir_types.h"
#include <stdbool.h>
#include <stddef.h>

// Loop structure
typedef struct ccvm_loop {
    ccvm_basic_block_t* header;
    ccvm_basic_block_t** blocks;
    size_t block_count;
    ccvm_basic_block_t** latches;
    size_t latch_count;
    ccvm_basic_block_t** exits;
    size_t exit_count;
    struct ccvm_loop** sub_loops;
    size_t sub_loop_count;
    struct ccvm_loop* parent;
    int depth;
    bool is_reducible;
    bool is_nested;
    size_t nesting_level;
} ccvm_loop_t;

// Loop information
typedef struct ccvm_loop_info {
    ccvm_function_t* function;
    ccvm_loop_t** loops;
    size_t loop_count;
    ccvm_loop_t* top_level_loop;
    bool has_irreducible_loops;
} ccvm_loop_info_t;

// Loop analysis
ccvm_loop_info_t* ccvm_analyze_loops(ccvm_function_t* function);
void ccvm_loop_info_destroy(ccvm_loop_info_t* loop_info);

// Loop queries
ccvm_loop_t* ccvm_loop_info_get_loop_for_block(ccvm_loop_info_t* loop_info, ccvm_basic_block_t* block);
ccvm_loop_t* ccvm_loop_info_get_parent_loop(ccvm_loop_info_t* loop_info, ccvm_loop_t* loop);
ccvm_loop_t** ccvm_loop_info_get_sub_loops(ccvm_loop_info_t* loop_info, ccvm_loop_t* loop, size_t* count);

// Loop properties
bool ccvm_loop_contains_block(ccvm_loop_t* loop, ccvm_basic_block_t* block);
bool ccvm_loop_contains_loop(ccvm_loop_t* outer, ccvm_loop_t* inner);
bool ccvm_loop_is_innermost(ccvm_loop_t* loop);
bool ccvm_loop_is_outmost(ccvm_loop_t* loop);
bool ccvm_loop_has_single_exit(ccvm_loop_t* loop);
bool ccvm_loop_has_single_latch(ccvm_loop_t* loop);
bool ccvm_loop_is_canonical(ccvm_loop_t* loop);

// Loop induction variables
typedef struct ccvm_induction_var {
    ccvm_value_t* value;
    ccvm_basic_block_t* header;
    int64_t initial_value;
    int64_t step;
    bool is_signed;
    bool is_wrapping;
} ccvm_induction_var_t;

ccvm_induction_var_t** ccvm_loop_get_induction_vars(ccvm_loop_t* loop, size_t* count);
void ccvm_induction_var_destroy(ccvm_induction_var_t* iv);

// Loop transformations
bool ccvm_loop_can_be_unrolled(ccvm_loop_t* loop, size_t factor);
bool ccvm_loop_can_be_vectorized(ccvm_loop_t* loop);
bool ccvm_loop_can_be_fusioned(ccvm_loop_t* loop1, ccvm_loop_t* loop2);
bool ccvm_loop_can_be_interchanged(ccvm_loop_t* loop, size_t level1, size_t level2);

// Loop bounds analysis
typedef struct ccvm_loop_bounds {
    int64_t min_trip_count;
    int64_t max_trip_count;
    int64_t avg_trip_count;
    bool is_countable;
    bool is_constant;
    ccvm_value_t* trip_count;
} ccvm_loop_bounds_t;

ccvm_loop_bounds_t* ccvm_loop_get_bounds(ccvm_loop_t* loop);
void ccvm_loop_bounds_destroy(ccvm_loop_bounds_t* bounds);

// Loop dependence analysis
typedef struct ccvm_loop_dependence {
    ccvm_instruction_t* source;
    ccvm_instruction_t* sink;
    size_t distance[3]; // Max 3D loops
    bool is_flow_dependence;
    bool is_anti_dependence;
    bool is_output_dependence;
    bool is_input_dependence;
    bool is_carried;
} ccvm_loop_dependence_t;

ccvm_loop_dependence_t** ccvm_loop_get_dependences(ccvm_loop_t* loop, size_t* count);
void ccvm_loop_dependence_destroy(ccvm_loop_dependence_t* dep);

// Utility functions
void ccvm_loop_print(ccvm_loop_t* loop, FILE* file);
void ccvm_loop_info_print(ccvm_loop_info_t* loop_info, FILE* file);
const char* ccvm_loop_type_to_string(ccvm_loop_t* loop);

#ifdef __cplusplus
}
#endif

#endif // CCVM_OPTIMIZER_LOOP_ANALYSIS_H