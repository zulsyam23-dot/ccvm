#include "ccvm/optimizer/pass_manager.h"
#include "ccvm/optimizer/ir_types.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

// Dead Code Elimination pass
// Menghapus instructions yang tidak digunakan (dead)

typedef struct dce_context {
    ccvm_function_t* function;
    HashSet* live_values;        // Values yang masih digunakan
    HashSet* dead_instructions;  // Instructions yang bisa dihapus
    size_t eliminated_count;     // Jumlah instructions yang dieliminasi
    size_t total_instructions;   // Total instructions yang diproses
} dce_context_t;

// HashSet implementation (sederhana, sama seperti di mem2reg)
typedef struct hash_entry {
    void* key;
    void* value;
    struct hash_entry* next;
} hash_entry_t;

typedef struct hash_map {
    hash_entry_t** buckets;
    size_t bucket_count;
    size_t size;
} hash_map_t;

typedef struct hash_set {
    hash_map_t* map;
} hash_set_t;

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
            free(entry);
            entry = next;
        }
    }
    
    free(map->buckets);
    free(map);
}

static size_t hash_pointer(const void* ptr) {
    return ((size_t)ptr >> 3) * 2654435761U;
}

static void hash_map_put(hash_map_t* map, void* key, void* value) {
    size_t hash = hash_pointer(key);
    size_t index = hash % map->bucket_count;
    
    hash_entry_t* entry = map->buckets[index];
    while (entry) {
        if (entry->key == key) {
            entry->value = value;
            return;
        }
        entry = entry->next;
    }
    
    hash_entry_t* new_entry = (hash_entry_t*)malloc(sizeof(hash_entry_t));
    new_entry->key = key;
    new_entry->value = value;
    new_entry->next = map->buckets[index];
    map->buckets[index] = new_entry;
    map->size++;
}

static void* hash_map_get(hash_map_t* map, void* key) {
    size_t hash = hash_pointer(key);
    size_t index = hash % map->bucket_count;
    
    hash_entry_t* entry = map->buckets[index];
    while (entry) {
        if (entry->key == key) {
            return entry->value;
        }
        entry = entry->next;
    }
    
    return NULL;
}

static hash_set_t* hash_set_create(size_t initial_size) {
    hash_set_t* set = (hash_set_t*)malloc(sizeof(hash_set_t));
    if (!set) return NULL;
    
    set->map = hash_map_create(initial_size);
    if (!set->map) {
        free(set);
        return NULL;
    }
    
    return set;
}

static void hash_set_destroy(hash_set_t* set) {
    if (!set) return;
    hash_map_destroy(set->map);
    free(set);
}

static void hash_set_add(hash_set_t* set, void* key) {
    hash_map_put(set->map, key, key);
}

static bool hash_set_contains(hash_set_t* set, void* key) {
    return hash_map_get(set->map, key) != NULL;
}

// Fungsi untuk mengecek apakah instruction memiliki side effects
static bool has_side_effects(ccvm_instruction_t* inst) {
    if (!inst) return false;
    
    enum ccvm_opcode opcode = ccvm_instruction_get_opcode(inst);
    
    // Instructions yang memiliki side effects
    switch (opcode) {
        case CCVM_OP_STORE:
        case CCVM_OP_CALL:
        case CCVM_OP_VAARG:
        case CCVM_OP_LANDINGPAD:
        case CCVM_OP_CATCHPAD:
        case CCVM_OP_CLEANUPPAD:
        case CCVM_OP_ATOMICCMPXCHG:
        case CCVM_OP_ATOMICRMW:
        case CCVM_OP_FENCE:
            return true;
            
        // Volatile operations
        default:
            // Cek flags untuk volatile operations
            return ccvm_instruction_may_have_side_effects(inst);
    }
}

// Fungsi untuk mengecek apakah instruction adalah terminator
static bool is_terminator(ccvm_instruction_t* inst) {
    if (!inst) return false;
    
    return ccvm_instruction_is_terminator(inst);
}

// Fungsi untuk mengecek apakah instruction digunakan
static bool is_instruction_used(ccvm_instruction_t* inst, dce_context_t* ctx) {
    if (!inst) return false;
    
    // Terminator instructions selalu digunakan
    if (is_terminator(inst)) {
        return true;
    }
    
    // Instructions dengan side effects selalu digunakan
    if (has_side_effects(inst)) {
        return true;
    }
    
    // Cek apakah value digunakan
    ccvm_value_t* value = (ccvm_value_t*)inst;
    if (value) {
        return hash_set_contains(ctx->live_values, value);
    }
    
    return false;
}

// Fungsi untuk menandai semua operands dari instruction sebagai live
static void mark_operands_live(ccvm_instruction_t* inst, dce_context_t* ctx) {
    if (!inst) return;
    
    size_t operand_count = ccvm_instruction_get_operand_count(inst);
    
    for (size_t i = 0; i < operand_count; i++) {
        ccvm_value_t* operand = ccvm_instruction_get_operand(inst, i);
        if (operand) {
            hash_set_add(ctx->live_values, operand);
        }
    }
}

// Fungsi untuk menandai phi operands sebagai live
static void mark_phi_operands_live(ccvm_basic_block_t* block, dce_context_t* ctx) {
    size_t inst_count = ccvm_basic_block_get_instruction_count(block);
    
    for (size_t i = 0; i < inst_count; i++) {
        ccvm_instruction_t* inst = ccvm_basic_block_get_instruction(block, i);
        
        if (ccvm_instruction_get_opcode(inst) == CCVM_OP_PHI) {
            // Phi nodes memiliki operands yang spesial
            // TODO: Implementasi phi operand marking
            mark_operands_live(inst, ctx);
        }
    }
}

// Iterative live value analysis
static void compute_live_values(dce_context_t* ctx) {
    bool changed = true;
    size_t iterations = 0;
    const size_t max_iterations = 1000;
    
    while (changed && iterations < max_iterations) {
        changed = false;
        iterations++;
        
        // Process all basic blocks
        size_t bb_count = ccvm_function_get_basic_block_count(ctx->function);
        
        for (size_t i = 0; i < bb_count; i++) {
            ccvm_basic_block_t* bb = ccvm_function_get_basic_block(ctx->function, i);
            
            // Process phi nodes first
            mark_phi_operands_live(bb, ctx);
            
            // Process all instructions in reverse order
            size_t inst_count = ccvm_basic_block_get_instruction_count(bb);
            
            for (size_t j = inst_count; j > 0; j--) {
                ccvm_instruction_t* inst = ccvm_basic_block_get_instruction(bb, j - 1);
                
                // Skip if already marked as dead
                if (hash_set_contains(ctx->dead_instructions, inst)) {
                    continue;
                }
                
                // Jika instruction digunakan, mark operands sebagai live
                if (is_instruction_used(inst, ctx)) {
                    size_t old_size = ctx->live_values->map->size;
                    mark_operands_live(inst, ctx);
                    
                    if (ctx->live_values->map->size > old_size) {
                        changed = true;
                    }
                }
            }
        }
    }
}

// Fungsi untuk mengidentifikasi dead instructions
static void identify_dead_instructions(dce_context_t* ctx) {
    size_t bb_count = ccvm_function_get_basic_block_count(ctx->function);
    
    for (size_t i = 0; i < bb_count; i++) {
        ccvm_basic_block_t* bb = ccvm_function_get_basic_block(ctx->function, i);
        size_t inst_count = ccvm_basic_block_get_instruction_count(bb);
        
        for (size_t j = 0; j < inst_count; j++) {
            ccvm_instruction_t* inst = ccvm_basic_block_get_instruction(bb, j);
            
            // Jika instruction tidak digunakan dan bukan terminator, mark sebagai dead
            if (!is_instruction_used(inst, ctx) && !is_terminator(inst)) {
                hash_set_add(ctx->dead_instructions, inst);
            }
        }
    }
}

// Fungsi untuk menghapus dead instructions
static void remove_dead_instructions(dce_context_t* ctx) {
    if (!ctx || !ctx->dead_instructions) return;

    size_t eliminated = 0;
    hash_map_t* map = ctx->dead_instructions->map;
    if (!map) {
        ctx->eliminated_count = 0;
        return;
    }

    // Iterate all buckets and unlink instructions from their parent basic blocks
    for (size_t b = 0; b < map->bucket_count; b++) {
        hash_entry_t* entry = map->buckets[b];
        while (entry) {
            ccvm_instruction_t* inst = (ccvm_instruction_t*)entry->key;
            if (inst) {
                ccvm_basic_block_t* parent = inst->parent;
                if (parent) {
                    // Unlink from linked list
                    if (inst->prev) inst->prev->next = inst->next;
                    if (inst->next) inst->next->prev = inst->prev;
                    if (parent->first_inst == inst) parent->first_inst = inst->next;
                    if (parent->last_inst == inst) parent->last_inst = inst->prev;
                    if (parent->terminator == inst) parent->terminator = NULL;
                }

                // Free operand array if present
                if (inst->operands) {
                    free(inst->operands);
                    inst->operands = NULL;
                }

                // Free instruction structure
                free(inst);
                eliminated++;
            }
            entry = entry->next;
        }
    }

    ctx->eliminated_count = eliminated;
}

// Main DCE pass function
static enum ccvm_pass_result dce_run(ccvm_pass_t* pass, void* ir_unit) {
    if (!pass || !ir_unit) {
        return CCVM_PASS_INVALID_INPUT;
    }
    
    ccvm_function_t* function = (ccvm_function_t*)ir_unit;
    
    // Initialize context
    dce_context_t ctx = {0};
    ctx.function = function;
    ctx.live_values = hash_set_create(128);
    ctx.dead_instructions = hash_set_create(128);
    
    if (!ctx.live_values || !ctx.dead_instructions) {
        hash_set_destroy(ctx.live_values);
        hash_set_destroy(ctx.dead_instructions);
        return CCVM_PASS_ERROR;
    }
    
    // Step 1: Hitung total instructions
    size_t bb_count = ccvm_function_get_basic_block_count(function);
    for (size_t i = 0; i < bb_count; i++) {
        ccvm_basic_block_t* bb = ccvm_function_get_basic_block(function, i);
        ctx.total_instructions += ccvm_basic_block_get_instruction_count(bb);
    }
    
    // Step 2: Compute live values (iterative)
    compute_live_values(&ctx);
    
    // Step 3: Identify dead instructions
    identify_dead_instructions(&ctx);
    
    // Step 4: Remove dead instructions
    remove_dead_instructions(&ctx);
    
    // Store statistics
    pass->private_data = malloc(sizeof(dce_context_t));
    if (pass->private_data) {
        memcpy(pass->private_data, &ctx, sizeof(dce_context_t));
    }
    
    // Cleanup temporary data structures
    hash_set_destroy(ctx.live_values);
    hash_set_destroy(ctx.dead_instructions);
    
    // Return success jika ada perubahan
    return ctx.eliminated_count > 0 ? CCVM_PASS_SUCCESS : CCVM_PASS_NO_CHANGE;
}

static void dce_initialize(ccvm_pass_t* pass) {
    // No initialization needed
}

static void dce_finalize(ccvm_pass_t* pass) {
    if (pass && pass->private_data) {
        free(pass->private_data);
        pass->private_data = NULL;
    }
}

static void dce_destroy(ccvm_pass_t* pass) {
    dce_finalize(pass);
}

static bool dce_is_required(ccvm_pass_t* pass, const void* ir_unit) {
    if (!pass || !ir_unit) {
        return false;
    }
    
    // DCE selalu berguna untuk membersihkan code
    return true;
}

static void dce_get_statistics(ccvm_pass_t* pass, void* stats) {
    if (!pass || !pass->private_data || !stats) {
        return;
    }
    
    dce_context_t* ctx = (dce_context_t*)pass->private_data;
    // Try to treat stats as ccvm_opt_stats if compatible
    struct ccvm_opt_stats* s = (struct ccvm_opt_stats*)stats;
    if (s) {
        s->instructions_before = ctx->total_instructions;
        s->instructions_after = ctx->total_instructions > ctx->eliminated_count ? ctx->total_instructions - ctx->eliminated_count : 0;
        s->passes_run = 1;
        s->analysis_time_ms = 0;
        s->transform_time_ms = 0;
        s->total_time_ms = 0.0;
    }
}

// Pass vtable
static const struct ccvm_pass_vtable dce_vtable = {
    .run = dce_run,
    .initialize = dce_initialize,
    .finalize = dce_finalize,
    .destroy = dce_destroy,
    .is_required = dce_is_required,
    .get_statistics = dce_get_statistics,
};

// Pass info
static const struct ccvm_pass_info dce_info = {
    .name = "dce",
    .description = "Dead Code Elimination - Remove unused instructions",
    .type = CCVM_PASS_FUNCTION,
    .strategy = CCVM_PASS_UNTIL_FIXED_POINT,
    .is_analysis_pass = false,
    .preserves_all = false,
    .modifies_cfg = false,
    .dependency_count = 1,
    .dependencies = (struct ccvm_pass_dependency[]){
        { .pass_name = "liveness", .required = true },
    },
};

// Create DCE pass
ccvm_pass_t* ccvm_create_dce_pass(void) {
    ccvm_pass_t* pass = (ccvm_pass_t*)calloc(1, sizeof(ccvm_pass_t));
    if (!pass) {
        return NULL;
    }
    
    pass->vtable = &dce_vtable;
    pass->info = &dce_info;
    pass->is_enabled = true;
    pass->is_initialized = false;
    pass->execution_count = 0;
    pass->total_time_ms = 0;
    pass->last_result = CCVM_PASS_SUCCESS;
    
    return pass;
}