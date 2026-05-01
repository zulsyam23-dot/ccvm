#ifndef CCVM_OPTIMIZER_CONFIG_H
#define CCVM_OPTIMIZER_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Optimizer configuration structure
struct ccvm_optimizer_config {
    // Optimization level
    enum ccvm_opt_level level;
    
    // Size vs speed trade-offs
    bool optimize_for_size;
    bool optimize_for_speed;
    
    // Individual optimizations
    bool enable_inlining;
    bool enable_loop_optimization;
    bool enable_vectorization;
    bool enable_unrolling;
    bool enable_interprocedural_optimization;
    bool enable_profile_guided_optimization;
    bool enable_link_time_optimization;
    
    // Analysis options
    bool enable_alias_analysis;
    bool enable_scalar_evolution;
    bool enable_value_tracking;
    bool enable_dom_tree_analysis;
    bool enable_loop_analysis;
    
    // Safety and verification
    bool verify_each_pass;
    bool preserve_debug_info;
    bool enable_sanitizers;
    bool enable_bounds_checking;
    
    // Resource limits
    size_t max_inline_threshold;
    size_t max_unroll_count;
    size_t max_vectorization_factor;
    size_t max_loop_iterations;
    double max_optimization_time_seconds;
    size_t max_memory_usage_mb;
    
    // Thresholds
    double code_growth_threshold;
    double compile_time_threshold;
    double size_threshold;
    double performance_threshold;
    
    // Debugging
    bool debug_passes;
    bool print_statistics;
    bool dump_ir_after_each_pass;
    bool trace_optimization;
    const char* dump_directory;
    
    // Target-specific
    const char* target_cpu;
    const char* target_features;
    bool enable_target_specific_optimizations;
    bool enable_simd_optimizations;
    bool enable_threading_optimizations;
    
    // Language-specific
    bool rust_lifetime_optimization;
    bool cpp_rtti_optimization;
    bool cpp_exceptions_optimization;
    bool java_gc_optimization;
    
    // Advanced options
    bool enable_aggressive_optimizations;
    bool enable_speculative_optimizations;
    bool enable_polyhedral_optimizations;
    bool enable_autovectorization;
    bool enable_superword_optimizations;
    bool enable_loop_distribution;
    bool enable_loop_fusion;
    bool enable_loop_interchange;
    bool enable_loop_tiling;
    bool enable_loop_unswitching;
    bool enable_loop_versioning;
    bool enable_loop_peeling;
    bool enable_loop_rotation;
    
    // Memory optimizations
    bool enable_memory_coalescing;
    bool enable_memory_prefetching;
    bool enable_cache_blocking;
    bool enable_memory_layout_optimization;
    
    // Parallelization
    bool enable_parallelization;
    bool enable_openmp_optimization;
    bool enable_mpi_optimization;
    bool enable_cuda_optimization;
    bool enable_opencl_optimization;
    
    // Profile-guided optimization settings
    struct {
        bool use_branch_probabilities;
        bool use_value_profiles;
        bool use_call_profiles;
        bool use_memory_profiles;
        bool use_loop_profiles;
        double hot_threshold;
        double cold_threshold;
        size_t profile_sample_rate;
    } profile_guided;
    
    // Machine learning optimization
    struct {
        bool enable_ml_optimization;
        const char* ml_model_path;
        bool enable_reinforcement_learning;
        bool enable_genetic_optimization;
        size_t ml_training_iterations;
    } machine_learning;
};

// Default configurations
const struct ccvm_optimizer_config* ccvm_optimizer_config_get_default(void);
const struct ccvm_optimizer_config* ccvm_optimizer_config_get_size_optimized(void);
const struct ccvm_optimizer_config* ccvm_optimizer_config_get_speed_optimized(void);
const struct ccvm_optimizer_config* ccvm_optimizer_config_get_debug(void);
const struct ccvm_optimizer_config* ccvm_optimizer_config_get_fast_compile(void);
const struct ccvm_optimizer_config* ccvm_optimizer_config_get_aggressive(void);

// Configuration validation
bool ccvm_optimizer_config_validate(const struct ccvm_optimizer_config* config, char** error_message);
void ccvm_optimizer_config_normalize(struct ccvm_optimizer_config* config);

// Configuration merging
void ccvm_optimizer_config_merge(
    struct ccvm_optimizer_config* target,
    const struct ccvm_optimizer_config* source,
    bool override_existing
);

// Configuration serialization
char* ccvm_optimizer_config_serialize(const struct ccvm_optimizer_config* config);
bool ccvm_optimizer_config_deserialize(struct ccvm_optimizer_config* config, const char* serialized);

// Configuration comparison
int ccvm_optimizer_config_compare(
    const struct ccvm_optimizer_config* config1,
    const struct ccvm_optimizer_config* config2
);

// Utility functions
void ccvm_optimizer_config_print(const struct ccvm_optimizer_config* config, FILE* file);
void ccvm_optimizer_config_set_target_defaults(
    struct ccvm_optimizer_config* config,
    const char* target_triple
);

#ifdef __cplusplus
}
#endif

#endif // CCVM_OPTIMIZER_CONFIG_H