#ifndef CCVM_OPTIMIZER_OPTIMIZER_H
#define CCVM_OPTIMIZER_OPTIMIZER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Forward declarations
typedef struct ccvm_module ccvm_module_t;
typedef struct ccvm_function ccvm_function_t;
typedef struct ccvm_basic_block ccvm_basic_block_t;
typedef struct ccvm_instruction ccvm_instruction_t;
typedef struct ccvm_optimizer ccvm_optimizer_t;
typedef struct ccvm_pass_manager ccvm_pass_manager_t;

// Optimization level
enum ccvm_opt_level {
    CCVM_OPT_NONE = 0,
    CCVM_OPT_BASIC = 1,
    CCVM_OPT_STANDARD = 2,
    CCVM_OPT_AGGRESSIVE = 3,
    CCVM_OPT_SIZE = 4,
    CCVM_OPT_SPEED = 5,
};

// Optimization statistics
struct ccvm_opt_stats {
    uint64_t instructions_before;
    uint64_t instructions_after;
    uint64_t basic_blocks_before;
    uint64_t basic_blocks_after;
    uint64_t functions_before;
    uint64_t functions_after;
    uint64_t passes_run;
    uint64_t analysis_time_ms;
    uint64_t transform_time_ms;
    double total_time_ms;
};

// Optimization result
enum ccvm_opt_result {
    CCVM_OPT_SUCCESS = 0,
    CCVM_OPT_ERROR_INVALID_INPUT = -1,
    CCVM_OPT_ERROR_OUT_OF_MEMORY = -2,
    CCVM_OPT_ERROR_INVALID_PASS = -3,
    CCVM_OPT_ERROR_ANALYSIS_FAILED = -4,
    CCVM_OPT_ERROR_TRANSFORM_FAILED = -5,
    CCVM_OPT_ERROR_VERIFICATION_FAILED = -6,
};

// Optimizer configuration
struct ccvm_optimizer_config {
    enum ccvm_opt_level level;
    bool enable_size_optimizations;
    bool enable_speed_optimizations;
    bool enable_loop_optimizations;
    bool enable_vectorization;
    bool enable_inlining;
    bool enable_interprocedural;
    bool enable_profile_guided;
    bool verify_each_pass;
    bool debug_passes;
    size_t max_iterations;
    size_t inline_threshold;
    size_t vectorization_threshold;
    double size_threshold; // Percentage threshold for code size changes
};

// Create optimizer
ccvm_optimizer_t* ccvm_optimizer_create(const struct ccvm_optimizer_config* config);

// Destroy optimizer
void ccvm_optimizer_destroy(ccvm_optimizer_t* optimizer);

// Optimize module
enum ccvm_opt_result ccvm_optimizer_run(
    ccvm_optimizer_t* optimizer,
    ccvm_module_t* module,
    struct ccvm_opt_stats* stats
);

// Optimize function
enum ccvm_opt_result ccvm_optimizer_run_on_function(
    ccvm_optimizer_t* optimizer,
    ccvm_function_t* function,
    struct ccvm_opt_stats* stats
);

// Get optimization statistics
void ccvm_optimizer_get_stats(
    const ccvm_optimizer_t* optimizer,
    struct ccvm_opt_stats* stats
);

// Reset optimizer state
void ccvm_optimizer_reset(ccvm_optimizer_t* optimizer);

// Enable/disable specific passes
void ccvm_optimizer_enable_pass(ccvm_optimizer_t* optimizer, const char* pass_name, bool enable);

// Set optimization level
void ccvm_optimizer_set_level(ccvm_optimizer_t* optimizer, enum ccvm_opt_level level);

// Get optimization level
enum ccvm_opt_level ccvm_optimizer_get_level(const ccvm_optimizer_t* optimizer);

// Check if pass is enabled
bool ccvm_optimizer_is_pass_enabled(const ccvm_optimizer_t* optimizer, const char* pass_name);

// Get list of available passes
const char** ccvm_optimizer_get_available_passes(const ccvm_optimizer_t* optimizer, size_t* count);

// Get pass description
const char* ccvm_optimizer_get_pass_description(const ccvm_optimizer_t* optimizer, const char* pass_name);

// Profile-guided optimization
enum ccvm_opt_result ccvm_optimizer_load_profile(
    ccvm_optimizer_t* optimizer,
    const char* profile_data,
    size_t profile_size
);

// Save optimization profile
enum ccvm_opt_result ccvm_optimizer_save_profile(
    const ccvm_optimizer_t* optimizer,
    char** profile_data,
    size_t* profile_size
);

// Utility functions
const char* ccvm_opt_result_to_string(enum ccvm_opt_result result);
const char* ccvm_opt_level_to_string(enum ccvm_opt_level level);

// Default configurations
const struct ccvm_optimizer_config* ccvm_optimizer_config_default(void);
const struct ccvm_optimizer_config* ccvm_optimizer_config_size(void);
const struct ccvm_optimizer_config* ccvm_optimizer_config_speed(void);
const struct ccvm_optimizer_config* ccvm_optimizer_config_debug(void);

#ifdef __cplusplus
}
#endif

#endif // CCVM_OPTIMIZER_OPTIMIZER_H