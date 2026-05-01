#ifndef CCVM_OPTIMIZER_PASSES_H
#define CCVM_OPTIMIZER_PASSES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "pass_manager.h"

// Pass creation functions (declared in pass_manager.h)
// Ini adalah header tambahan untuk dokumentasi dan kemudahan penggunaan

// Core optimization passes
#define CCVM_PASS_MEM2REG "mem2reg"
#define CCVM_PASS_DCE "dce"
#define CCVM_PASS_CONST_FOLDING "const_folding"
#define CCVM_PASS_INLINING "inlining"
#define CCVM_PASS_LOOP_OPT "loop_opt"
#define CCVM_PASS_VECTORIZATION "vectorization"

// Analysis passes
#define CCVM_PASS_DOM_TREE "dom_tree"
#define CCVM_PASS_LOOP_ANALYSIS "loop_analysis"
#define CCVM_PASS_ALIAS_ANALYSIS "alias_analysis"
#define CCVM_PASS_SCALAR_EVOLUTION "scalar_evolution"
#define CCVM_PASS_VALUE_TRACKING "value_tracking"
#define CCVM_PASS_LIVENESS "liveness"

// Transformation passes
#define CCVM_PASS_CFG_SIMPLIFICATION "cfg_simplification"
#define CCVM_PASS_GVN "gvn"
#define CCVM_PASS_INDUCTION_VAR "induction_var"
#define CCVM_PASS_LOOP_UNROLL "loop_unroll"
#define CCVM_PASS_LOOP_VECTORIZE "loop_vectorize"
#define CCVM_PASS_SLP_VECTORIZE "slp_vectorize"
#define CCVM_PASS_INSTRUCTION_COMBINE "instruction_combine"
#define CCVM_PASS_AGGRESSIVE_DCE "aggressive_dce"
#define CCVM_PASS_GLOBAL_OPT "global_opt"

// Interprocedural passes
#define CCVM_PASS_IP_CONSTANT_PROP "ip_constant_prop"
#define CCVM_PASS_IP_CPROP "ip_cprop"
#define CCVM_PASS_DEAD_ARG_ELIMINATION "dead_arg_elimination"
#define CCVM_PASS_FUNCTION_ATTRS "function_attrs"
#define CCVM_PASS_INLINE_COST_ANALYSIS "inline_cost_analysis"
#define CCVM_PASS_ARG_PROMOTION "arg_promotion"
#define CCVM_PASS_TAIL_CALL_ELIMINATION "tail_call_elimination"

// Standard optimization pipelines
enum ccvm_pipeline_type {
    CCVM_PIPELINE_O0,  // No optimization
    CCVM_PIPELINE_O1,  // Basic optimization
    CCVM_PIPELINE_O2,  // Standard optimization
    CCVM_PIPELINE_O3,  // Aggressive optimization
    CCVM_PIPELINE_Os,  // Optimize for size
    CCVM_PIPELINE_Oz,  // Optimize for size aggressively
    CCVM_PIPELINE_FAST, // Fast compilation
    CCVM_PIPELINE_PIPE, // Pipe-friendly optimization
};

// Create standard optimization pipeline
ccvm_pass_manager_t* ccvm_create_pipeline(enum ccvm_pipeline_type type);

// Create custom optimization pipeline
ccvm_pass_manager_t* ccvm_create_custom_pipeline(
    const char** pass_names,
    size_t pass_count,
    enum ccvm_pass_strategy strategy
);

// Pipeline configuration
struct ccvm_pipeline_config {
    enum ccvm_pipeline_type type;
    bool enable_size_opts;
    bool enable_speed_opts;
    bool enable_loop_opts;
    bool enable_vectorization;
    bool enable_inlining;
    bool enable_ip_opts;
    bool enable_profile_guided;
    bool run_until_fixed_point;
    size_t max_iterations;
    double timeout_seconds;
};

// Create pipeline with configuration
ccvm_pass_manager_t* ccvm_create_pipeline_with_config(
    const struct ccvm_pipeline_config* config
);

// Predefined pass sequences
const char** ccvm_get_o1_pass_sequence(size_t* count);
const char** ccvm_get_o2_pass_sequence(size_t* count);
const char** ccvm_get_o3_pass_sequence(size_t* count);
const char** ccvm_get_os_pass_sequence(size_t* count);
const char** ccvm_get_oz_pass_sequence(size_t* count);

// Pass ordering utilities
bool ccvm_validate_pass_order(
    const char** passes,
    size_t pass_count,
    char** error_message
);

// Pass dependency resolution
enum ccvm_opt_result ccvm_resolve_pass_dependencies(
    const char** passes,
    size_t pass_count,
    const char*** resolved_passes,
    size_t* resolved_count
);

// Pipeline statistics
struct ccvm_pipeline_stats {
    size_t total_passes;
    size_t analysis_passes;
    size_t transform_passes;
    size_t iterations;
    double total_time_ms;
    double analysis_time_ms;
    double transform_time_ms;
    size_t functions_processed;
    size_t basic_blocks_processed;
    size_t instructions_processed;
    size_t instructions_before;
    size_t instructions_after;
    size_t basic_blocks_before;
    size_t basic_blocks_after;
};

// Get pipeline statistics
void ccvm_get_pipeline_stats(
    const ccvm_pass_manager_t* pipeline,
    struct ccvm_pipeline_stats* stats
);

// Pipeline optimization
enum ccvm_opt_result ccvm_optimize_pipeline(
    ccvm_pass_manager_t* pipeline,
    enum ccvm_pipeline_type target_type
);

// Profile-guided pipeline optimization
enum ccvm_opt_result ccvm_optimize_pipeline_with_profile(
    ccvm_pass_manager_t* pipeline,
    const char* profile_data,
    size_t profile_size
);

// Pipeline comparison
int ccvm_compare_pipelines(
    const ccvm_pass_manager_t* pipeline1,
    const ccvm_pass_manager_t* pipeline2,
    struct ccvm_pipeline_stats* stats1,
    struct ccvm_pipeline_stats* stats2
);

// Pipeline serialization
enum ccvm_opt_result ccvm_serialize_pipeline(
    const ccvm_pass_manager_t* pipeline,
    char** serialized_data,
    size_t* serialized_size
);

enum ccvm_opt_result ccvm_deserialize_pipeline(
    ccvm_pass_manager_t* pipeline,
    const char* serialized_data,
    size_t serialized_size
);

// Utility functions
const char* ccvm_pipeline_type_to_string(enum ccvm_pipeline_type type);
enum ccvm_pipeline_type ccvm_string_to_pipeline_type(const char* str);
bool ccvm_is_analysis_pass(const char* pass_name);
bool ccvm_is_transform_pass(const char* pass_name);
bool ccvm_is_ip_pass(const char* pass_name);
bool ccvm_is_loop_pass(const char* pass_name);
bool ccvm_is_vectorization_pass(const char* pass_name);

// Pass categories
enum ccvm_pass_category {
    CCVM_PASS_CAT_ANALYSIS,
    CCVM_PASS_CAT_TRANSFORM,
    CCVM_PASS_CAT_IP,
    CCVM_PASS_CAT_LOOP,
    CCVM_PASS_CAT_VECTORIZATION,
    CCVM_PASS_CAT_MEMORY,
    CCVM_PASS_CAT_CONTROL_FLOW,
    CCVM_PASS_CAT_MISC,
};

// Get pass category
enum ccvm_pass_category ccvm_get_pass_category(const char* pass_name);

// Get passes in category
const char** ccvm_get_passes_in_category(
    enum ccvm_pass_category category,
    size_t* count
);

#ifdef __cplusplus
}
#endif

#endif // CCVM_OPTIMIZER_PASSES_H