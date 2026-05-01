#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "ccvm/optimizer/optimizer.h"
#include "ccvm/optimizer/pass_manager.h"
#include "ccvm/optimizer/passes.h"

// Test helper functions
static void print_test_header(const char* test_name) {
    printf("\n=== Testing: %s ===\n", test_name);
}

static void print_test_result(const char* test_name, bool passed) {
    printf("%s: %s\n", test_name, passed ? "PASSED" : "FAILED");
}

static void print_stats(const struct ccvm_opt_stats* stats) {
    printf("Optimization Statistics:\n");
    printf("  Instructions: %llu -> %llu\n", 
           (unsigned long long)stats->instructions_before,
           (unsigned long long)stats->instructions_after);
    printf("  Basic Blocks: %llu -> %llu\n",
           (unsigned long long)stats->basic_blocks_before,
           (unsigned long long)stats->basic_blocks_after);
    printf("  Functions: %llu -> %llu\n",
           (unsigned long long)stats->functions_before,
           (unsigned long long)stats->functions_after);
    printf("  Passes Run: %llu\n", (unsigned long long)stats->passes_run);
    printf("  Total Time: %.2f ms\n", stats->total_time_ms);
}

// Test optimizer creation and destruction
static bool test_optimizer_create_destroy(void) {
    print_test_header("Optimizer Create/Destroy");
    
    struct ccvm_optimizer_config config = {
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
        .size_threshold = 0.05,
    };
    
    ccvm_optimizer_t* optimizer = ccvm_optimizer_create(&config);
    if (!optimizer) {
        printf("Failed to create optimizer\n");
        return false;
    }
    
    // Test basic operations
    enum ccvm_opt_level level = ccvm_optimizer_get_level(optimizer);
    if (level != CCVM_OPT_STANDARD) {
        printf("Expected optimization level STANDARD, got %d\n", level);
        ccvm_optimizer_destroy(optimizer);
        return false;
    }
    
    // Test pass enabling/disabling
    ccvm_optimizer_enable_pass(optimizer, "mem2reg", false);
    if (ccvm_optimizer_is_pass_enabled(optimizer, "mem2reg")) {
        printf("Expected mem2reg to be disabled\n");
        ccvm_optimizer_destroy(optimizer);
        return false;
    }
    
    ccvm_optimizer_enable_pass(optimizer, "mem2reg", true);
    if (!ccvm_optimizer_is_pass_enabled(optimizer, "mem2reg")) {
        printf("Expected mem2reg to be enabled\n");
        ccvm_optimizer_destroy(optimizer);
        return false;
    }
    
    ccvm_optimizer_destroy(optimizer);
    return true;
}

// Test default configurations
static bool test_default_configs(void) {
    print_test_header("Default Configurations");
    
    // Test default config
    const struct ccvm_optimizer_config* default_config = ccvm_optimizer_config_default();
    if (!default_config) {
        printf("Failed to get default config\n");
        return false;
    }
    
    if (default_config->level != CCVM_OPT_STANDARD) {
        printf("Expected STANDARD level in default config\n");
        return false;
    }
    
    // Test size config
    const struct ccvm_optimizer_config* size_config = ccvm_optimizer_config_size();
    if (!size_config) {
        printf("Failed to get size config\n");
        return false;
    }
    
    if (size_config->level != CCVM_OPT_SIZE) {
        printf("Expected SIZE level in size config\n");
        return false;
    }
    
    // Test speed config
    const struct ccvm_optimizer_config* speed_config = ccvm_optimizer_config_speed();
    if (!speed_config) {
        printf("Failed to get speed config\n");
        return false;
    }
    
    if (speed_config->level != CCVM_OPT_AGGRESSIVE) {
        printf("Expected AGGRESSIVE level in speed config\n");
        return false;
    }
    
    // Test debug config
    const struct ccvm_optimizer_config* debug_config = ccvm_optimizer_config_debug();
    if (!debug_config) {
        printf("Failed to get debug config\n");
        return false;
    }
    
    if (debug_config->level != CCVM_OPT_NONE) {
        printf("Expected NONE level in debug config\n");
        return false;
    }
    
    return true;
}

// Test pass manager
static bool test_pass_manager(void) {
    print_test_header("Pass Manager");
    
    struct ccvm_pass_manager_config pm_config = {
        .max_passes = 50,
        .enable_verification = false,
        .enable_statistics = true,
        .enable_debug_output = false,
        .preserve_analysis_results = true,
        .timeout_seconds = 300.0,
    };
    
    ccvm_pass_manager_t* manager = ccvm_pass_manager_create(&pm_config);
    if (!manager) {
        printf("Failed to create pass manager\n");
        return false;
    }
    
    // Create some passes
    ccvm_pass_t* mem2reg_pass = ccvm_create_mem2reg_pass();
    ccvm_pass_t* dce_pass = ccvm_create_dce_pass();
    ccvm_pass_t* const_folding_pass = ccvm_create_const_folding_pass();
    
    if (!mem2reg_pass || !dce_pass || !const_folding_pass) {
        printf("Failed to create passes\n");
        ccvm_pass_manager_destroy(manager);
        return false;
    }
    
    // Add passes to manager
    if (ccvm_pass_manager_add_pass(manager, mem2reg_pass) != CCVM_OPT_SUCCESS) {
        printf("Failed to add mem2reg pass\n");
        ccvm_pass_manager_destroy(manager);
        return false;
    }
    
    if (ccvm_pass_manager_add_pass(manager, dce_pass) != CCVM_OPT_SUCCESS) {
        printf("Failed to add dce pass\n");
        ccvm_pass_manager_destroy(manager);
        return false;
    }
    
    if (ccvm_pass_manager_add_pass(manager, const_folding_pass) != CCVM_OPT_SUCCESS) {
        printf("Failed to add const_folding pass\n");
        ccvm_pass_manager_destroy(manager);
        return false;
    }
    
    // Test pass lookup
    ccvm_pass_t* found_pass = ccvm_pass_manager_get_pass(manager, "mem2reg");
    if (found_pass != mem2reg_pass) {
        printf("Failed to find mem2reg pass\n");
        ccvm_pass_manager_destroy(manager);
        return false;
    }
    
    // Test pass enabling/disabling
    ccvm_pass_manager_enable_pass(manager, "mem2reg", false);
    if (ccvm_pass_manager_is_pass_enabled(manager, "mem2reg")) {
        printf("Expected mem2reg to be disabled\n");
        ccvm_pass_manager_destroy(manager);
        return false;
    }
    
    ccvm_pass_manager_enable_pass(manager, "mem2reg", true);
    if (!ccvm_pass_manager_is_pass_enabled(manager, "mem2reg")) {
        printf("Expected mem2reg to be enabled\n");
        ccvm_pass_manager_destroy(manager);
        return false;
    }
    
    ccvm_pass_manager_destroy(manager);
    return true;
}

// Test optimization levels
static bool test_optimization_levels(void) {
    print_test_header("Optimization Levels");
    
    // Test each optimization level
    enum ccvm_opt_level levels[] = {
        CCVM_OPT_NONE,
        CCVM_OPT_BASIC,
        CCVM_OPT_STANDARD,
        CCVM_OPT_AGGRESSIVE,
        CCVM_OPT_SIZE,
        CCVM_OPT_SPEED
    };
    
    const char* level_names[] = {
        "None",
        "Basic",
        "Standard", 
        "Aggressive",
        "Size",
        "Speed"
    };
    
    for (size_t i = 0; i < sizeof(levels)/sizeof(levels[0]); i++) {
        struct ccvm_optimizer_config config = {
            .level = levels[i],
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
            .size_threshold = 0.05,
        };
        
        ccvm_optimizer_t* optimizer = ccvm_optimizer_create(&config);
        if (!optimizer) {
            printf("Failed to create optimizer for level %s\n", level_names[i]);
            return false;
        }
        
        enum ccvm_opt_level actual_level = ccvm_optimizer_get_level(optimizer);
        if (actual_level != levels[i]) {
            printf("Expected level %s, got %d\n", level_names[i], actual_level);
            ccvm_optimizer_destroy(optimizer);
            return false;
        }
        
        ccvm_optimizer_destroy(optimizer);
    }
    
    return true;
}

// Test utility functions
static bool test_utility_functions(void) {
    print_test_header("Utility Functions");
    
    // Test result to string conversion
    const char* result_str = ccvm_opt_result_to_string(CCVM_OPT_SUCCESS);
    if (strcmp(result_str, "Success") != 0) {
        printf("Expected 'Success', got '%s'\n", result_str);
        return false;
    }
    
    result_str = ccvm_opt_result_to_string(CCVM_OPT_ERROR_INVALID_INPUT);
    if (strcmp(result_str, "Invalid input") != 0) {
        printf("Expected 'Invalid input', got '%s'\n", result_str);
        return false;
    }
    
    // Test level to string conversion
    const char* level_str = ccvm_opt_level_to_string(CCVM_OPT_STANDARD);
    if (strcmp(level_str, "Standard") != 0) {
        printf("Expected 'Standard', got '%s'\n", level_str);
        return false;
    }
    
    level_str = ccvm_opt_level_to_string(CCVM_OPT_AGGRESSIVE);
    if (strcmp(level_str, "Aggressive") != 0) {
        printf("Expected 'Aggressive', got '%s'\n", level_str);
        return false;
    }
    
    return true;
}

// Test pass creation
static bool test_pass_creation(void) {
    print_test_header("Pass Creation");
    
    // Test creating individual passes
    ccvm_pass_t* passes[] = {
        ccvm_create_mem2reg_pass(),
        ccvm_create_dce_pass(),
        ccvm_create_const_folding_pass(),
        ccvm_create_dom_tree_pass(),
        ccvm_create_loop_analysis_pass(),
        ccvm_create_alias_analysis_pass(),
        ccvm_create_scalar_evolution_pass(),
        ccvm_create_value_tracking_pass(),
        ccvm_create_cfg_simplification_pass(),
        ccvm_create_gvn_pass(),
        ccvm_create_inlining_pass(),
        ccvm_create_aggressive_dce_pass(),
    };
    
    size_t pass_count = sizeof(passes) / sizeof(passes[0]);
    
    for (size_t i = 0; i < pass_count; i++) {
        if (!passes[i]) {
            printf("Failed to create pass %zu\n", i);
            
            // Cleanup created passes
            for (size_t j = 0; j < i; j++) {
                if (passes[j]) {
                    // Note: We need a proper pass destruction function
                    free(passes[j]);
                }
            }
            return false;
        }
    }
    
    // Cleanup
    for (size_t i = 0; i < pass_count; i++) {
        if (passes[i]) {
            free(passes[i]);
        }
    }
    
    return true;
}

// Test optimization pipeline
static bool test_optimization_pipeline(void) {
    print_test_header("Optimization Pipeline");
    
    struct ccvm_optimizer_config config = {
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
        .size_threshold = 0.05,
    };
    
    ccvm_optimizer_t* optimizer = ccvm_optimizer_create(&config);
    if (!optimizer) {
        printf("Failed to create optimizer\n");
        return false;
    }
    
    // Test creating different pipelines
    struct ccvm_pass_manager_config pm_config = {
        .max_passes = 100,
        .enable_verification = false,
        .enable_statistics = true,
        .enable_debug_output = false,
        .preserve_analysis_results = true,
        .timeout_seconds = 300.0,
    };
    
    ccvm_pass_manager_t* manager = ccvm_pass_manager_create(&pm_config);
    if (!manager) {
        printf("Failed to create pass manager\n");
        ccvm_optimizer_destroy(optimizer);
        return false;
    }
    
    // Create standard pipeline
    enum ccvm_opt_result result = ccvm_pass_manager_create_pipeline(manager, CCVM_OPT_STANDARD);
    if (result != CCVM_OPT_SUCCESS) {
        printf("Failed to create standard pipeline: %s\n", ccvm_opt_result_to_string(result));
        ccvm_pass_manager_destroy(manager);
        ccvm_optimizer_destroy(optimizer);
        return false;
    }
    
    ccvm_pass_manager_destroy(manager);
    ccvm_optimizer_destroy(optimizer);
    return true;
}

// Main test runner
int main(void) {
    printf("CCVM Optimizer Test Suite\n");
    printf("==========================\n");
    
    // Run all tests
    struct {
        const char* name;
        bool (*test_func)(void);
    } tests[] = {
        {"Optimizer Create/Destroy", test_optimizer_create_destroy},
        {"Default Configurations", test_default_configs},
        {"Pass Manager", test_pass_manager},
        {"Optimization Levels", test_optimization_levels},
        {"Utility Functions", test_utility_functions},
        {"Pass Creation", test_pass_creation},
        {"Optimization Pipeline", test_optimization_pipeline},
    };
    
    size_t test_count = sizeof(tests) / sizeof(tests[0]);
    size_t passed_count = 0;
    
    clock_t start_time = clock();
    
    for (size_t i = 0; i < test_count; i++) {
        bool passed = tests[i].test_func();
        print_test_result(tests[i].name, passed);
        
        if (passed) {
            passed_count++;
        }
    }
    
    clock_t end_time = clock();
    double total_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    
    printf("\n==========================\n");
    printf("Test Summary:\n");
    printf("  Total Tests: %zu\n", test_count);
    printf("  Passed: %zu\n", passed_count);
    printf("  Failed: %zu\n", test_count - passed_count);
    printf("  Success Rate: %.1f%%\n", (double)passed_count / test_count * 100);
    printf("  Total Time: %.3f seconds\n", total_time);
    
    return (passed_count == test_count) ? 0 : 1;
}