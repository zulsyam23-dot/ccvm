/**
 * @file optimizer.c
 * @brief CCVM Optimizer main implementation
 * @author CCVM Team
 * @date 2026
 */

#include "ccvm/optimizer/optimizer.h"
#include "ccvm/optimizer/pass_manager.h"
#include "ccvm/optimizer/ir_types.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>

/**
 * @brief Structure untuk internal state optimizer
 */
struct ccvm_optimizer {
    struct ccvm_optimizer_config config;
    ccvm_pass_manager_t* pass_manager;
    struct ccvm_opt_stats stats;
    bool is_initialized;
    void* profile_data;
    size_t profile_size;
};

// Default configurations
static const struct ccvm_optimizer_config default_config = {
    .level = CCVM_OPT_STANDARD,
    .enable_size_optimizations = true,
    .enable_speed_optimizations = true,
    .enable_loop_optimizations = true,
    .enable_vectorization = true,
    .enable_inlining = true,
    .enable_interprocedural = true,
    .enable_profile_guided = false,
    .verify_each_pass = false,
    .debug_passes = false,
    .max_iterations = 1000,
    .inline_threshold = 200,
    .vectorization_threshold = 16,
    .size_threshold = 0.05, // 5% threshold
};

static const struct ccvm_optimizer_config size_config = {
    .level = CCVM_OPT_SIZE,
    .enable_size_optimizations = true,
    .enable_speed_optimizations = false,
    .enable_loop_optimizations = true,
    .enable_vectorization = false,
    .enable_inlining = true,
    .enable_interprocedural = true,
    .enable_profile_guided = false,
    .verify_each_pass = false,
    .debug_passes = false,
    .max_iterations = 500,
    .inline_threshold = 50,
    .vectorization_threshold = 0,
    .size_threshold = 0.01, // 1% threshold
};

static const struct ccvm_optimizer_config speed_config = {
    .level = CCVM_OPT_AGGRESSIVE,
    .enable_size_optimizations = false,
    .enable_speed_optimizations = true,
    .enable_loop_optimizations = true,
    .enable_vectorization = true,
    .enable_inlining = true,
    .enable_interprocedural = true,
    .enable_profile_guided = true,
    .verify_each_pass = false,
    .debug_passes = false,
    .max_iterations = 2000,
    .inline_threshold = 500,
    .vectorization_threshold = 8,
    .size_threshold = 0.10, // 10% threshold
};

static const struct ccvm_optimizer_config debug_config = {
    .level = CCVM_OPT_NONE,
    .enable_size_optimizations = false,
    .enable_speed_optimizations = false,
    .enable_loop_optimizations = false,
    .enable_vectorization = false,
    .enable_inlining = false,
    .enable_interprocedural = false,
    .enable_profile_guided = false,
    .verify_each_pass = true,
    .debug_passes = true,
    .max_iterations = 1,
    .inline_threshold = 0,
    .vectorization_threshold = 0,
    .size_threshold = 0.0,
};

// Create optimizer
ccvm_optimizer_t* ccvm_optimizer_create(const struct ccvm_optimizer_config* config) {
    ccvm_optimizer_t* optimizer = (ccvm_optimizer_t*)calloc(1, sizeof(ccvm_optimizer_t));
    if (!optimizer) {
        return NULL;
    }
    
    // Copy configuration
    if (config) {
        optimizer->config = *config;
    } else {
        optimizer->config = default_config;
    }
    
    // Create pass manager
    struct ccvm_pass_manager_config pm_config = {
        .max_passes = 100,
        .enable_verification = optimizer->config.verify_each_pass,
        .enable_statistics = true,
        .enable_debug_output = optimizer->config.debug_passes,
        .preserve_analysis_results = true,
        .timeout_seconds = 300.0,
    };
    
    optimizer->pass_manager = ccvm_pass_manager_create(&pm_config);
    if (!optimizer->pass_manager) {
        free(optimizer);
        return NULL;
    }
    
    // Create standard pipeline
    if (ccvm_pass_manager_create_pipeline(optimizer->pass_manager, optimizer->config.level) != CCVM_OPT_SUCCESS) {
        ccvm_pass_manager_destroy(optimizer->pass_manager);
        free(optimizer);
        return NULL;
    }
    
    optimizer->is_initialized = true;
    return optimizer;
}

// Destroy optimizer
void ccvm_optimizer_destroy(ccvm_optimizer_t* optimizer) {
    if (!optimizer) {
        return;
    }
    
    if (optimizer->pass_manager) {
        ccvm_pass_manager_destroy(optimizer->pass_manager);
    }
    
    if (optimizer->profile_data) {
        free(optimizer->profile_data);
    }
    
    free(optimizer);
}

// Optimize module
enum ccvm_opt_result ccvm_optimizer_run(
    ccvm_optimizer_t* optimizer,
    ccvm_module_t* module,
    struct ccvm_opt_stats* stats) {
    
    if (!optimizer || !optimizer->is_initialized || !module) {
        return CCVM_OPT_ERROR_INVALID_INPUT;
    }
    
    clock_t start_time = clock();
    
    // Reset statistics
    memset(&optimizer->stats, 0, sizeof(optimizer->stats));
    
    // Collect initial statistics
    optimizer->stats.functions_before = ccvm_module_get_function_count(module);
    optimizer->stats.instructions_before = 0;
    for (size_t i = 0; i < optimizer->stats.functions_before; i++) {
        ccvm_function_t* func = ccvm_module_get_function(module, i);
        optimizer->stats.instructions_before += ccvm_function_get_instruction_count(func);
    }
    
    // Run pass manager
    enum ccvm_opt_result result = ccvm_pass_manager_run_on_module(
        optimizer->pass_manager,
        module,
        &optimizer->stats
    );
    
    if (result != CCVM_OPT_SUCCESS) {
        return result;
    }
    
    // Collect final statistics
    optimizer->stats.functions_after = ccvm_module_get_function_count(module);
    optimizer->stats.instructions_after = 0;
    for (size_t i = 0; i < optimizer->stats.functions_after; i++) {
        ccvm_function_t* func = ccvm_module_get_function(module, i);
        optimizer->stats.instructions_after += ccvm_function_get_instruction_count(func);
    }
    
    // Calculate timing
    clock_t end_time = clock();
    optimizer->stats.total_time_ms = (double)(end_time - start_time) * 1000.0 / CLOCKS_PER_SEC;
    
    // Copy statistics if requested
    if (stats) {
        *stats = optimizer->stats;
    }
    
    return CCVM_OPT_SUCCESS;
}

// Optimize function
enum ccvm_opt_result ccvm_optimizer_run_on_function(
    ccvm_optimizer_t* optimizer,
    ccvm_function_t* function,
    struct ccvm_opt_stats* stats) {
    
    if (!optimizer || !optimizer->is_initialized || !function) {
        return CCVM_OPT_ERROR_INVALID_INPUT;
    }
    
    clock_t start_time = clock();
    
    // Reset statistics for this run
    struct ccvm_opt_stats local_stats = {0};
    
    // Run pass manager on function
    enum ccvm_opt_result result = ccvm_pass_manager_run_on_function(
        optimizer->pass_manager,
        function,
        &local_stats
    );
    
    if (result != CCVM_OPT_SUCCESS) {
        return result;
    }
    
    // Calculate timing
    clock_t end_time = clock();
    local_stats.total_time_ms = (double)(end_time - start_time) * 1000.0 / CLOCKS_PER_SEC;
    
    // Copy statistics if requested
    if (stats) {
        *stats = local_stats;
    }
    
    return CCVM_OPT_SUCCESS;
}

// Get optimization statistics
void ccvm_optimizer_get_stats(
    const ccvm_optimizer_t* optimizer,
    struct ccvm_opt_stats* stats) {
    
    if (!optimizer || !stats) {
        return;
    }
    
    *stats = optimizer->stats;
}

// Reset optimizer state
void ccvm_optimizer_reset(ccvm_optimizer_t* optimizer) {
    if (!optimizer || !optimizer->is_initialized) {
        return;
    }
    
    // Reset pass manager
    ccvm_pass_manager_reset(optimizer->pass_manager);
    
    // Reset statistics
    memset(&optimizer->stats, 0, sizeof(optimizer->stats));
}

// Enable/disable specific passes
void ccvm_optimizer_enable_pass(ccvm_optimizer_t* optimizer, const char* pass_name, bool enable) {
    if (!optimizer || !optimizer->is_initialized || !pass_name) {
        return;
    }
    
    ccvm_pass_manager_enable_pass(optimizer->pass_manager, pass_name, enable);
}

// Set optimization level
void ccvm_optimizer_set_level(ccvm_optimizer_t* optimizer, enum ccvm_opt_level level) {
    if (!optimizer || !optimizer->is_initialized) {
        return;
    }
    
    optimizer->config.level = level;
    
    // Recreate pipeline with new level
    ccvm_pass_manager_reset(optimizer->pass_manager);
    ccvm_pass_manager_create_pipeline(optimizer->pass_manager, level);
}

// Get optimization level
enum ccvm_opt_level ccvm_optimizer_get_level(const ccvm_optimizer_t* optimizer) {
    if (!optimizer) {
        return CCVM_OPT_NONE;
    }
    
    return optimizer->config.level;
}

// Check if pass is enabled
bool ccvm_optimizer_is_pass_enabled(const ccvm_optimizer_t* optimizer, const char* pass_name) {
    if (!optimizer || !optimizer->is_initialized || !pass_name) {
        return false;
    }
    
    return ccvm_pass_manager_is_pass_enabled(optimizer->pass_manager, pass_name);
}

// Get list of available passes
const char** ccvm_optimizer_get_available_passes(const ccvm_optimizer_t* optimizer, size_t* count) {
    if (!optimizer || !optimizer->is_initialized || !count) {
        return NULL;
    }
    
    // TODO: Implement getting available passes from pass manager
    *count = 0;
    return NULL;
}

// Get pass description
const char* ccvm_optimizer_get_pass_description(const ccvm_optimizer_t* optimizer, const char* pass_name) {
    if (!optimizer || !optimizer->is_initialized || !pass_name) {
        return NULL;
    }
    
    // TODO: Implement getting pass description from pass manager
    return NULL;
}

// Profile-guided optimization
enum ccvm_opt_result ccvm_optimizer_load_profile(
    ccvm_optimizer_t* optimizer,
    const char* profile_data,
    size_t profile_size) {
    
    if (!optimizer || !profile_data || profile_size == 0) {
        return CCVM_OPT_ERROR_INVALID_INPUT;
    }
    
    // Free existing profile data
    if (optimizer->profile_data) {
        free(optimizer->profile_data);
        optimizer->profile_data = NULL;
        optimizer->profile_size = 0;
    }
    
    // Copy new profile data
    optimizer->profile_data = (char*)malloc(profile_size);
    if (!optimizer->profile_data) {
        return CCVM_OPT_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(optimizer->profile_data, profile_data, profile_size);
    optimizer->profile_size = profile_size;
    
    // TODO: Parse profile data and update optimization decisions
    
    return CCVM_OPT_SUCCESS;
}

// Save optimization profile
enum ccvm_opt_result ccvm_optimizer_save_profile(
    const ccvm_optimizer_t* optimizer,
    char** profile_data,
    size_t* profile_size) {
    
    if (!optimizer || !profile_data || !profile_size) {
        return CCVM_OPT_ERROR_INVALID_INPUT;
    }
    
    // TODO: Generate profile data from optimization statistics
    
    *profile_data = NULL;
    *profile_size = 0;
    
    return CCVM_OPT_SUCCESS;
}

// Utility functions
const char* ccvm_opt_result_to_string(enum ccvm_opt_result result) {
    switch (result) {
        case CCVM_OPT_SUCCESS: return "Success";
        case CCVM_OPT_ERROR_INVALID_INPUT: return "Invalid input";
        case CCVM_OPT_ERROR_OUT_OF_MEMORY: return "Out of memory";
        case CCVM_OPT_ERROR_INVALID_PASS: return "Invalid pass";
        case CCVM_OPT_ERROR_ANALYSIS_FAILED: return "Analysis failed";
        case CCVM_OPT_ERROR_TRANSFORM_FAILED: return "Transform failed";
        case CCVM_OPT_ERROR_VERIFICATION_FAILED: return "Verification failed";
        default: return "Unknown error";
    }
}

const char* ccvm_opt_level_to_string(enum ccvm_opt_level level) {
    switch (level) {
        case CCVM_OPT_NONE: return "None";
        case CCVM_OPT_BASIC: return "Basic";
        case CCVM_OPT_STANDARD: return "Standard";
        case CCVM_OPT_AGGRESSIVE: return "Aggressive";
        case CCVM_OPT_SIZE: return "Size";
        case CCVM_OPT_SPEED: return "Speed";
        default: return "Unknown";
    }
}

// Default configurations
const struct ccvm_optimizer_config* ccvm_optimizer_config_default(void) {
    return &default_config;
}

const struct ccvm_optimizer_config* ccvm_optimizer_config_size(void) {
    return &size_config;
}

const struct ccvm_optimizer_config* ccvm_optimizer_config_speed(void) {
    return &speed_config;
}

const struct ccvm_optimizer_config* ccvm_optimizer_config_debug(void) {
    return &debug_config;
}