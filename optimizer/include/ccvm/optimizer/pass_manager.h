#ifndef CCVM_OPTIMIZER_PASS_MANAGER_H
#define CCVM_OPTIMIZER_PASS_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "optimizer.h"

// Forward declarations
typedef struct ccvm_pass ccvm_pass_t;
typedef struct ccvm_pass_manager ccvm_pass_manager_t;

// Pass types
enum ccvm_pass_type {
    CCVM_PASS_MODULE = 0,
    CCVM_PASS_FUNCTION = 1,
    CCVM_PASS_BASIC_BLOCK = 2,
    CCVM_PASS_INSTRUCTION = 3,
};

// Pass execution strategy
enum ccvm_pass_strategy {
    CCVM_PASS_ONCE = 0,
    CCVM_PASS_UNTIL_FIXED_POINT = 1,
    CCVM_PASS_ITERATIVE = 2,
};

// Pass dependencies
struct ccvm_pass_dependency {
    const char* pass_name;
    bool required; // If true, pass must run before this pass
};

// Pass information
struct ccvm_pass_info {
    const char* name;
    const char* description;
    enum ccvm_pass_type type;
    enum ccvm_pass_strategy strategy;
    bool is_analysis_pass;
    bool preserves_all;
    bool modifies_cfg;
    size_t dependency_count;
    const struct ccvm_pass_dependency* dependencies;
};

// Pass result
enum ccvm_pass_result {
    CCVM_PASS_SUCCESS = 0,
    CCVM_PASS_NO_CHANGE = 1,
    CCVM_PASS_ERROR = -1,
    CCVM_PASS_INVALID_INPUT = -2,
};

// Pass manager configuration
struct ccvm_pass_manager_config {
    size_t max_passes;
    bool enable_verification;
    bool enable_statistics;
    bool enable_debug_output;
    bool preserve_analysis_results;
    double timeout_seconds;
};

// Pass interface
struct ccvm_pass_vtable {
    enum ccvm_pass_result (*run)(ccvm_pass_t* pass, void* ir_unit);
    void (*initialize)(ccvm_pass_t* pass);
    void (*finalize)(ccvm_pass_t* pass);
    void (*destroy)(ccvm_pass_t* pass);
    bool (*is_required)(ccvm_pass_t* pass, const void* ir_unit);
    void (*get_statistics)(ccvm_pass_t* pass, void* stats);
};

// Pass structure
struct ccvm_pass {
    const struct ccvm_pass_vtable* vtable;
    const struct ccvm_pass_info* info;
    void* private_data;
    bool is_enabled;
    bool is_initialized;
    uint64_t execution_count;
    uint64_t total_time_ms;
    enum ccvm_pass_result last_result;
};

// Create pass manager
ccvm_pass_manager_t* ccvm_pass_manager_create(const struct ccvm_pass_manager_config* config);

// Destroy pass manager
void ccvm_pass_manager_destroy(ccvm_pass_manager_t* manager);

// Add pass to manager
enum ccvm_opt_result ccvm_pass_manager_add_pass(
    ccvm_pass_manager_t* manager,
    ccvm_pass_t* pass
);

// Remove pass from manager
enum ccvm_opt_result ccvm_pass_manager_remove_pass(
    ccvm_pass_manager_t* manager,
    const char* pass_name
);

// Run all passes on module
enum ccvm_opt_result ccvm_pass_manager_run_on_module(
    ccvm_pass_manager_t* manager,
    ccvm_module_t* module,
    struct ccvm_opt_stats* stats
);

// Run all passes on function
enum ccvm_opt_result ccvm_pass_manager_run_on_function(
    ccvm_pass_manager_t* manager,
    ccvm_function_t* function,
    struct ccvm_opt_stats* stats
);

// Get pass by name
ccvm_pass_t* ccvm_pass_manager_get_pass(
    ccvm_pass_manager_t* manager,
    const char* pass_name
);

// Enable/disable pass
void ccvm_pass_manager_enable_pass(
    ccvm_pass_manager_t* manager,
    const char* pass_name,
    bool enable
);

// Check if pass is enabled
bool ccvm_pass_manager_is_pass_enabled(
    const ccvm_pass_manager_t* manager,
    const char* pass_name
);

// Get pass execution statistics
void ccvm_pass_manager_get_pass_stats(
    const ccvm_pass_manager_t* manager,
    const char* pass_name,
    uint64_t* execution_count,
    uint64_t* total_time_ms,
    enum ccvm_pass_result* last_result
);

// Reset pass manager state
void ccvm_pass_manager_reset(ccvm_pass_manager_t* manager);

// Validate pass dependencies
enum ccvm_opt_result ccvm_pass_manager_validate_dependencies(
    const ccvm_pass_manager_t* manager
);

// Create standard pass pipeline
enum ccvm_opt_result ccvm_pass_manager_create_pipeline(
    ccvm_pass_manager_t* manager,
    enum ccvm_opt_level level
);

// Built-in passes
ccvm_pass_t* ccvm_create_mem2reg_pass(void);
ccvm_pass_t* ccvm_create_dce_pass(void);
ccvm_pass_t* ccvm_create_const_folding_pass(void);
ccvm_pass_t* ccvm_create_inlining_pass(void);
ccvm_pass_t* ccvm_create_loop_opt_pass(void);
ccvm_pass_t* ccvm_create_vectorization_pass(void);
ccvm_pass_t* ccvm_create_alias_analysis_pass(void);
ccvm_pass_t* ccvm_create_dom_tree_pass(void);
ccvm_pass_t* ccvm_create_loop_analysis_pass(void);
ccvm_pass_t* ccvm_create_scalar_evolution_pass(void);
ccvm_pass_t* ccvm_create_value_tracking_pass(void);
ccvm_pass_t* ccvm_create_cfg_simplification_pass(void);
ccvm_pass_t* ccvm_create_gvn_pass(void);
ccvm_pass_t* ccvm_create_liveness_pass(void);
ccvm_pass_t* ccvm_create_induction_var_pass(void);
ccvm_pass_t* ccvm_create_loop_unroll_pass(void);
ccvm_pass_t* ccvm_create_loop_vectorize_pass(void);
ccvm_pass_t* ccvm_create_slp_vectorize_pass(void);
ccvm_pass_t* ccvm_create_instruction_combine_pass(void);
ccvm_pass_t* ccvm_create_aggressive_dce_pass(void);
ccvm_pass_t* ccvm_create_global_opt_pass(void);
ccvm_pass_t* ccvm_create_ip_constant_prop_pass(void);
ccvm_pass_t* ccvm_create_ip_cprop_pass(void);
ccvm_pass_t* ccvm_create_dead_arg_elimination_pass(void);
ccvm_pass_t* ccvm_create_function_attrs_pass(void);
ccvm_pass_t* ccvm_create_inline_cost_analysis_pass(void);
ccvm_pass_t* ccvm_create_arg_promotion_pass(void);
ccvm_pass_t* ccvm_create_tail_call_elimination_pass(void);
ccvm_pass_t* ccvm_create_reassociate_pass(void);
ccvm_pass_t* ccvm_create_demote_memory_to_register_pass(void);
ccvm_pass_t* ccvm_create_promote_memory_to_register_pass(void);
ccvm_pass_t* ccvm_create_memcpy_opt_pass(void);
ccvm_pass_t* ccvm_create_simplify_lib_calls_pass(void);
ccvm_pass_t* ccvm_create_jump_threading_pass(void);
ccvm_pass_t* ccvm_create_correlated_value_propagation_pass(void);
ccvm_pass_t* ccvm_create_early_cse_pass(void);
ccvm_pass_t* ccvm_create_lower_expect_intrinsic_pass(void);
ccvm_pass_t* ccvm_create_type_based_alias_analysis_pass(void);
ccvm_pass_t* ccvm_create_basic_alias_analysis_pass(void);
ccvm_pass_t* ccvm_create_globals_mod_ref_pass(void);
ccvm_pass_t* ccvm_create_iv_users_pass(void);
ccvm_pass_t* ccvm_create_licm_pass(void);
ccvm_pass_t* ccvm_create_loop_sink_pass(void);
ccvm_pass_t* ccvm_create_loop_rotation_pass(void);
ccvm_pass_t* ccvm_create_loop_idiom_pass(void);
ccvm_pass_t* ccvm_create_loop_deletion_pass(void);
ccvm_pass_t* ccvm_create_simple_loop_unswitch_pass(void);
ccvm_pass_t* ccvm_create_scalar_repl_aggregates_pass(void);
ccvm_pass_t* ccvm_create_lower_switch_pass(void);
ccvm_pass_t* ccvm_create_lower_invoke_pass(void);
ccvm_pass_t* ccvm_create_lower_allocations_pass(void);
ccvm_pass_t* ccvm_create_strip_dead_prototypes_pass(void);
ccvm_pass_t* ccvm_create_strip_symbols_pass(void);
ccvm_pass_t* ccvm_create_merge_functions_pass(void);
ccvm_pass_t* ccvm_create_internalize_pass(void);
ccvm_pass_t* ccvm_create_partial_inlining_pass(void);
ccvm_pass_t* ccvm_create_ipsccp_pass(void);
ccvm_pass_t* ccvm_create_sink_pass(void);
ccvm_pass_t* ccvm_create_lower_atomic_pass(void);
ccvm_pass_t* ccvm_create_place_safepoints_pass(void);
ccvm_pass_t* ccvm_create_lower_guard_intrinsic_pass(void);
ccvm_pass_t* ccvm_create_callsite_splitting_pass(void);
ccvm_pass_t* ccvm_create_hot_cold_splitting_pass(void);
ccvm_pass_t* ccvm_create_lower_type_tests_pass(void);
ccvm_pass_t* ccvm_create_lower_constant_intrinsics_pass(void);
ccvm_pass_t* ccvm_create_alignment_from_assumptions_pass(void);
ccvm_pass_t* ccvm_create_strip_nondebug_symbols_pass(void);
ccvm_pass_t* ccvm_create_ee_instrument_pass(void);
ccvm_pass_t* ccvm_create_address_sanitizer_pass(void);
ccvm_pass_t* ccvm_create_memory_sanitizer_pass(void);
ccvm_pass_t* ccvm_create_thread_sanitizer_pass(void);
ccvm_pass_t* ccvm_create_dataflow_sanitizer_pass(void);
ccvm_pass_t* ccvm_create_bounds_checking_pass(void);
ccvm_pass_t* ccvm_create_unify_function_exit_nodes_pass(void);
ccvm_pass_t* ccvm_create_machinelicm_pass(void);
ccvm_pass_t* ccvm_create_machine_cse_pass(void);
ccvm_pass_t* ccvm_create_machine_sinking_pass(void);
ccvm_pass_t* ccvm_create_peephole_pass(void);
ccvm_pass_t* ccvm_create_dead_machine_code_elimination_pass(void);
ccvm_pass_t* ccvm_create_stack_slot_coloring_pass(void);
ccvm_pass_t* ccvm_create_local_stack_slot_allocation_pass(void);
ccvm_pass_t* ccvm_create_spill_code_placement_pass(void);
ccvm_pass_t* ccvm_create_two_address_instruction_pass(void);
ccvm_pass_t* ccvm_create_register_coalescer_pass(void);
ccvm_pass_t* ccvm_create_machine_block_frequency_pass(void);
ccvm_pass_t* ccvm_create_machine_branch_probability_pass(void);
ccvm_pass_t* ccvm_create_edge_splitter_pass(void);
ccvm_pass_t* ccvm_create_branch_relaxation_pass(void);
ccvm_pass_t* ccvm_create_post_ra_machine_sinking_pass(void);
ccvm_pass_t* ccvm_create_shrink_wrapping_pass(void);
ccvm_pass_t* ccvm_create_prologepilog_inserter_pass(void);
ccvm_pass_t* ccvm_create_implicit_null_checks_pass(void);
ccvm_pass_t* ccvm_create_machine_block_placement_pass(void);
ccvm_pass_t* ccvm_create_x86_mmx_pass(void);
ccvm_pass_t* ccvm_create_x86_sse2_pass(void);
ccvm_pass_t* ccvm_create_x86_sse42_pass(void);
ccvm_pass_t* ccvm_create_x86_avx_pass(void);
ccvm_pass_t* ccvm_create_x86_avx2_pass(void);
ccvm_pass_t* ccvm_create_x86_avx512_pass(void);
ccvm_pass_t* ccvm_create_x86_avx512bf16_pass(void);
ccvm_pass_t* ccvm_create_x86_avx512bitalg_pass(void);
ccvm_pass_t* ccvm_create_x86_avx512bw_pass(void);
ccvm_pass_t* ccvm_create_x86_avx512cd_pass(void);
ccvm_pass_t* ccvm_create_x86_avx512dq_pass(void);
ccvm_pass_t* ccvm_create_x86_avx512er_pass(void);
ccvm_pass_t* ccvm_create_x86_avx512f_pass(void);
ccvm_pass_t* ccvm_create_x86_avx512ifma_pass(void);
ccvm_pass_t* ccvm_create_x86_avx512pf_pass(void);
ccvm_pass_t* ccvm_create_x86_avx512vbmi_pass(void);
ccvm_pass_t* ccvm_create_x86_avx512vbmi2_pass(void);
ccvm_pass_t* ccvm_create_x86_avx512vl_pass(void);
ccvm_pass_t* ccvm_create_x86_avx512vnni_pass(void);
ccvm_pass_t* ccvm_create_x86_avx512vp2intersect_pass(void);
ccvm_pass_t* ccvm_create_x86_avx512vpclmulqdq_pass(void);
ccvm_pass_t* ccvm_create_x86_avx512vpopcntdq_pass(void);

// Analysis passes
ccvm_pass_t* ccvm_create_alias_set_tracker_pass(void);
ccvm_pass_t* ccvm_create_phi_values_pass(void);
ccvm_pass_t* ccvm_create_iv_simplify_pass(void);
ccvm_pass_t* ccvm_create_constant_range_pass(void);
ccvm_pass_t* ccvm_create_target_passes_pass(void);
ccvm_pass_t* ccvm_create_target_transform_info_pass(void);
ccvm_pass_t* ccvm_create_block_frequency_pass(void);
ccvm_pass_t* ccvm_create_branch_probability_pass(void);
ccvm_pass_t* ccvm_create_assumption_cache_tracker_pass(void);
ccvm_pass_t* ccvm_create_target_library_info_pass(void);
ccvm_pass_t* ccvm_create_memory_ssa_pass(void);
ccvm_pass_t* ccvm_create_memory_ssa_wrapper_pass(void);

// Utility functions
const char* ccvm_pass_result_to_string(enum ccvm_pass_result result);
const char* ccvm_pass_type_to_string(enum ccvm_pass_type type);
const char* ccvm_pass_strategy_to_string(enum ccvm_pass_strategy strategy);
bool ccvm_pass_is_analysis(const ccvm_pass_t* pass);
bool ccvm_pass_is_transform(const ccvm_pass_t* pass);
const struct ccvm_pass_info* ccvm_pass_get_info(const ccvm_pass_t* pass);
void ccvm_pass_set_private_data(ccvm_pass_t* pass, void* data);
void* ccvm_pass_get_private_data(const ccvm_pass_t* pass);

#ifdef __cplusplus
}
#endif

#endif // CCVM_OPTIMIZER_PASS_MANAGER_H