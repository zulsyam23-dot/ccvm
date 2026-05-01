#include "ccvm/optimizer/pass_manager.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "ccvm/optimizer/ir_verifier.h"

// Pass manager implementation
struct ccvm_pass_manager {
    struct ccvm_pass_manager_config config;
    ccvm_pass_t** passes;
    size_t pass_count;
    size_t pass_capacity;
    HashMap* pass_map;          // pass_name -> pass
    HashMap* analysis_cache;    // analysis results cache
    uint64_t* pass_stats;       // execution statistics
    bool is_running;
    clock_t start_time;
    double elapsed_time;
};

// HashMap implementation (sederhana)
typedef struct hash_entry {
    char* key;
    void* value;
    struct hash_entry* next;
} hash_entry_t;

typedef struct hash_map {
    hash_entry_t** buckets;
    size_t bucket_count;
    size_t size;
} hash_map_t;

static hash_map_t* hash_map_create(size_t initial_size) {
    hash_map_t* map = (hash_map_t*)calloc(1, sizeof(hash_map_t));
    if (!map) return NULL;
    
    map->bucket_count = initial_size;
    map->buckets = (hash_entry_t**)calloc(initial_size, sizeof(hash_entry_t*));
    if (!map->buckets) {
        free(map);
        return NULL;
    }
    
    return map;
}

static void hash_map_destroy(hash_map_t* map) {
    if (!map) return;
    
    for (size_t i = 0; i < map->bucket_count; i++) {
        hash_entry_t* entry = map->buckets[i];
        while (entry) {
            hash_entry_t* next = entry->next;
            free(entry->key);
            free(entry);
            entry = next;
        }
    }
    
    free(map->buckets);
    free(map);
}

static size_t hash_string(const char* str) {
    size_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

static void hash_map_put(hash_map_t* map, const char* key, void* value) {
    size_t hash = hash_string(key);
    size_t index = hash % map->bucket_count;
    
    hash_entry_t* entry = map->buckets[index];
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            entry->value = value;
            return;
        }
        entry = entry->next;
    }
    
    hash_entry_t* new_entry = (hash_entry_t*)malloc(sizeof(hash_entry_t));
    new_entry->key = strdup(key);
    new_entry->value = value;
    new_entry->next = map->buckets[index];
    map->buckets[index] = new_entry;
    map->size++;
}

static void* hash_map_get(hash_map_t* map, const char* key) {
    size_t hash = hash_string(key);
    size_t index = hash % map->bucket_count;
    
    hash_entry_t* entry = map->buckets[index];
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            return entry->value;
        }
        entry = entry->next;
    }
    
    return NULL;
}

static bool hash_map_contains(hash_map_t* map, const char* key) {
    return hash_map_get(map, key) != NULL;
}

// Create pass manager
ccvm_pass_manager_t* ccvm_pass_manager_create(const struct ccvm_pass_manager_config* config) {
    ccvm_pass_manager_t* manager = (ccvm_pass_manager_t*)calloc(1, sizeof(ccvm_pass_manager_t));
    if (!manager) return NULL;
    
    // Copy configuration
    if (config) {
        manager->config = *config;
    } else {
        // Default configuration
        manager->config.max_passes = 100;
        manager->config.enable_verification = false;
        manager->config.enable_statistics = true;
        manager->config.enable_debug_output = false;
        manager->config.preserve_analysis_results = true;
        manager->config.timeout_seconds = 300.0;
    }
    
    // Initialize data structures
    manager->pass_capacity = 16;
    manager->passes = (ccvm_pass_t**)calloc(manager->pass_capacity, sizeof(ccvm_pass_t*));
    if (!manager->passes) {
        free(manager);
        return NULL;
    }
    
    manager->pass_map = hash_map_create(32);
    manager->analysis_cache = hash_map_create(32);
    manager->pass_stats = (uint64_t*)calloc(manager->pass_capacity, sizeof(uint64_t));
    
    if (!manager->pass_map || !manager->analysis_cache || !manager->pass_stats) {
        ccvm_pass_manager_destroy(manager);
        return NULL;
    }
    
    manager->is_running = false;
    manager->elapsed_time = 0.0;
    
    return manager;
}

// Destroy pass manager
void ccvm_pass_manager_destroy(ccvm_pass_manager_t* manager) {
    if (!manager) return;
    
    // Destroy all passes
    for (size_t i = 0; i < manager->pass_count; i++) {
        if (manager->passes[i]) {
            if (manager->passes[i]->vtable && manager->passes[i]->vtable->destroy) {
                manager->passes[i]->vtable->destroy(manager->passes[i]);
            }
        }
    }
    
    free(manager->passes);
    free(manager->pass_stats);
    hash_map_destroy(manager->pass_map);
    hash_map_destroy(manager->analysis_cache);
    free(manager);
}

// Add pass to manager
enum ccvm_opt_result ccvm_pass_manager_add_pass(ccvm_pass_manager_t* manager, ccvm_pass_t* pass) {
    if (!manager || !pass) {
        return CCVM_OPT_ERROR_INVALID_INPUT;
    }
    
    if (manager->pass_count >= manager->pass_capacity) {
        // Resize pass array
        size_t new_capacity = manager->pass_capacity * 2;
        ccvm_pass_t** new_passes = (ccvm_pass_t**)realloc(manager->passes, new_capacity * sizeof(ccvm_pass_t*));
        uint64_t* new_stats = (uint64_t*)realloc(manager->pass_stats, new_capacity * sizeof(uint64_t));
        
        if (!new_passes || !new_stats) {
            return CCVM_OPT_ERROR_OUT_OF_MEMORY;
        }
        
        manager->passes = new_passes;
        manager->pass_stats = new_stats;
        manager->pass_capacity = new_capacity;
    }
    
    // Add pass
    manager->passes[manager->pass_count] = pass;
    manager->pass_stats[manager->pass_count] = 0;
    manager->pass_count++;
    
    // Add to pass map
    if (pass->info && pass->info->name) {
        hash_map_put(manager->pass_map, pass->info->name, pass);
    }
    
    return CCVM_OPT_SUCCESS;
}

// Remove pass from manager
enum ccvm_opt_result ccvm_pass_manager_remove_pass(ccvm_pass_manager_t* manager, const char* pass_name) {
    if (!manager || !pass_name) {
        return CCVM_OPT_ERROR_INVALID_INPUT;
    }
    
    // Find pass by name
    ccvm_pass_t* pass = (ccvm_pass_t*)hash_map_get(manager->pass_map, pass_name);
    if (!pass) {
        return CCVM_OPT_ERROR_INVALID_PASS;
    }
    
    // Find index in passes array
    size_t index = 0;
    for (; index < manager->pass_count; index++) {
        if (manager->passes[index] == pass) {
            break;
        }
    }
    
    if (index >= manager->pass_count) {
        return CCVM_OPT_ERROR_INVALID_PASS;
    }
    
    // Remove from pass map
    // Note: We can't easily remove from hash map, so we just mark as disabled
    pass->is_enabled = false;
    
    return CCVM_OPT_SUCCESS;
}

// Run all passes on module
enum ccvm_opt_result ccvm_pass_manager_run_on_module(ccvm_pass_manager_t* pm, void* module, struct ccvm_opt_stats* stats) {
    if (!pm || !module) return CCVM_OPT_ERROR_INVALID_INPUT;
    
    printf("[PassManager] Running optimization pipeline on module...\n");
    
    for (size_t i = 0; i < pm->pass_count; i++) {
        ccvm_pass_t* pass = pm->passes[i];
        if (!pass->is_enabled) continue;
        
        printf("  - Running pass: %s\n", pass->info->name);
        
        enum ccvm_pass_result result = pass->vtable->run(pass, (ccvm_function_t*)module);
        if (result != CCVM_PASS_SUCCESS) {
            fprintf(stderr, "Error: Pass '%s' failed with result %d\n", pass->info->name, result);
            return CCVM_OPT_ERROR_TRANSFORM_FAILED;
        }
    }
    
    return CCVM_OPT_SUCCESS;
}

// Run all passes on function
enum ccvm_opt_result ccvm_pass_manager_run_on_function(
    ccvm_pass_manager_t* manager,
    ccvm_function_t* function,
    struct ccvm_opt_stats* stats) {
    
    if (!manager || !function) {
        return CCVM_OPT_ERROR_INVALID_INPUT;
    }
    
    // Check timeout
    if (manager->config.timeout_seconds > 0) {
        double elapsed = (double)(clock() - manager->start_time) / CLOCKS_PER_SEC;
        if (elapsed > manager->config.timeout_seconds) {
            return CCVM_OPT_ERROR_ANALYSIS_FAILED;
        }
    }
    
    // Run each pass
    for (size_t i = 0; i < manager->pass_count; i++) {
        ccvm_pass_t* pass = manager->passes[i];
        
        if (!pass || !pass->is_enabled) {
            continue;
        }
        
        // Check if pass is applicable to this IR unit
        if (pass->vtable && pass->vtable->is_required) {
            if (!pass->vtable->is_required(pass, function)) {
                continue;
            }
        }
        
        // Initialize pass if needed
        if (!pass->is_initialized && pass->vtable && pass->vtable->initialize) {
            pass->vtable->initialize(pass);
            pass->is_initialized = true;
        }
        
        // Run pass
        clock_t pass_start = clock();
        enum ccvm_pass_result result = CCVM_PASS_SUCCESS;
        
        if (pass->vtable && pass->vtable->run) {
            result = pass->vtable->run(pass, function);
        }
        
        clock_t pass_end = clock();
        pass->total_time_ms += (pass_end - pass_start) * 1000 / CLOCKS_PER_SEC;
        pass->execution_count++;
        pass->last_result = result;
        
        // Handle pass result
        switch (result) {
            case CCVM_PASS_SUCCESS:
                // Pass made changes
                if (stats) {
                    stats->passes_run++;
                }
                break;
                
            case CCVM_PASS_NO_CHANGE:
                // Pass didn't make changes
                break;
                
            case CCVM_PASS_ERROR:
            case CCVM_PASS_INVALID_INPUT:
                return CCVM_OPT_ERROR_TRANSFORM_FAILED;
                
            default:
                break;
        }
        
        // Verify if requested
        if (manager->config.enable_verification) {
            int ok = ccvm_ir_verify(function);
            if (!ok) {
                return CCVM_OPT_ERROR_VERIFICATION_FAILED;
            }
        }
    }
    
    return CCVM_OPT_SUCCESS;
}

// Get pass by name
ccvm_pass_t* ccvm_pass_manager_get_pass(ccvm_pass_manager_t* manager, const char* pass_name) {
    if (!manager || !pass_name) {
        return NULL;
    }
    
    return (ccvm_pass_t*)hash_map_get(manager->pass_map, pass_name);
}

// Enable/disable pass
void ccvm_pass_manager_enable_pass(ccvm_pass_manager_t* manager, const char* pass_name, bool enable) {
    if (!manager || !pass_name) {
        return;
    }
    
    ccvm_pass_t* pass = (ccvm_pass_t*)hash_map_get(manager->pass_map, pass_name);
    if (pass) {
        pass->is_enabled = enable;
    }
}

// Check if pass is enabled
bool ccvm_pass_manager_is_pass_enabled(const ccvm_pass_manager_t* manager, const char* pass_name) {
    if (!manager || !pass_name) {
        return false;
    }
    
    ccvm_pass_t* pass = (ccvm_pass_t*)hash_map_get(manager->pass_map, pass_name);
    return pass ? pass->is_enabled : false;
}

// Get pass execution statistics
void ccvm_pass_manager_get_pass_stats(
    const ccvm_pass_manager_t* manager,
    const char* pass_name,
    uint64_t* execution_count,
    uint64_t* total_time_ms,
    enum ccvm_pass_result* last_result) {
    
    if (!manager || !pass_name) {
        return;
    }
    
    // Find pass by name
    ccvm_pass_t* pass = (ccvm_pass_t*)hash_map_get(manager->pass_map, pass_name);
    if (!pass) {
        return;
    }
    
    // Find index in passes array
    size_t index = 0;
    for (; index < manager->pass_count; index++) {
        if (manager->passes[index] == pass) {
            break;
        }
    }
    
    if (index >= manager->pass_count) {
        return;
    }
    
    if (execution_count) {
        *execution_count = pass->execution_count;
    }
    
    if (total_time_ms) {
        *total_time_ms = pass->total_time_ms;
    }
    
    if (last_result) {
        *last_result = pass->last_result;
    }
}

// Reset pass manager state
void ccvm_pass_manager_reset(ccvm_pass_manager_t* manager) {
    if (!manager) {
        return;
    }
    
    // Reset all passes
    for (size_t i = 0; i < manager->pass_count; i++) {
        ccvm_pass_t* pass = manager->passes[i];
        if (pass) {
            pass->is_initialized = false;
            pass->execution_count = 0;
            pass->total_time_ms = 0;
            pass->last_result = CCVM_PASS_SUCCESS;
        }
    }
    
    // Clear analysis cache
    hash_map_destroy(manager->analysis_cache);
    manager->analysis_cache = hash_map_create(32);
    
    manager->elapsed_time = 0.0;
}

// Validate pass dependencies
enum ccvm_opt_result ccvm_pass_manager_validate_dependencies(const ccvm_pass_manager_t* manager) {
    if (!manager) {
        return CCVM_OPT_ERROR_INVALID_INPUT;
    }
    
    // TODO: Implement dependency validation
    // This would check that required passes are available and in correct order
    
    return CCVM_OPT_SUCCESS;
}

// Create standard pass pipeline
ccvm_pass_manager_t* ccvm_pass_manager_create_pipeline(int opt_level) {
    ccvm_pass_manager_t* pm = ccvm_pass_manager_create(NULL);
    if (!pm) return NULL;
    
    // Add passes based on optimization level
    if (opt_level >= 1) {
        ccvm_pass_manager_add_pass(pm, ccvm_create_const_folding_pass());
        ccvm_pass_manager_add_pass(pm, ccvm_create_dce_pass());
    }
    
    if (opt_level >= 2) {
        ccvm_pass_manager_add_pass(pm, ccvm_create_inlining_pass());
        ccvm_pass_manager_add_pass(pm, ccvm_create_mem2reg_pass());
    }
    
    return pm;
}

// Utility functions
const char* ccvm_pass_result_to_string(enum ccvm_pass_result result) {
    switch (result) {
        case CCVM_PASS_SUCCESS: return "Success";
        case CCVM_PASS_NO_CHANGE: return "No change";
        case CCVM_PASS_ERROR: return "Error";
        case CCVM_PASS_INVALID_INPUT: return "Invalid input";
        default: return "Unknown";
    }
}

const char* ccvm_pass_type_to_string(enum ccvm_pass_type type) {
    switch (type) {
        case CCVM_PASS_MODULE: return "Module";
        case CCVM_PASS_FUNCTION: return "Function";
        case CCVM_PASS_BASIC_BLOCK: return "Basic Block";
        case CCVM_PASS_INSTRUCTION: return "Instruction";
        default: return "Unknown";
    }
}

const char* ccvm_pass_strategy_to_string(enum ccvm_pass_strategy strategy) {
    switch (strategy) {
        case CCVM_PASS_ONCE: return "Once";
        case CCVM_PASS_UNTIL_FIXED_POINT: return "Until Fixed Point";
        case CCVM_PASS_ITERATIVE: return "Iterative";
        default: return "Unknown";
    }
}