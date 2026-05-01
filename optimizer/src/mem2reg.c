#include "ccvm/optimizer/pass_manager.h"
#include "ccvm/optimizer/ir_types.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

// Mem2Reg pass - mengkonversi memory operations ke SSA register form
// Ini adalah optimasi penting yang mengeliminasi alloca/load/store instructions

typedef struct mem2reg_context {
    ccvm_function_t* function;
    HashMap* alloca_map;        // alloca -> phi nodes
    HashMap* def_map;           // alloca -> definitions
    HashMap* use_map;           // alloca -> uses
    HashSet* promoted_allocas;  // allocas yang berhasil dipromosikan
    size_t phi_count;           // jumlah phi nodes yang dibuat
    size_t alloca_count;        // jumlah allocas yang diproses
    size_t eliminated_count;    // jumlah allocas yang dieliminasi
} mem2reg_context_t;

// HashMap implementation (sederhana)
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

static bool hash_map_contains(hash_map_t* map, void* key) {
    return hash_map_get(map, key) != NULL;
}

// HashSet implementation (sederhana)
typedef struct hash_set {
    hash_map_t* map;
} hash_set_t;

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
    return hash_map_contains(set->map, key);
}

// Fungsi untuk mengecek apakah instruction adalah alloca
static bool is_alloca_instruction(ccvm_instruction_t* inst) {
    return inst && ccvm_instruction_get_opcode(inst) == CCVM_OP_ALLOCA;
}

// Fungsi untuk mengecek apakah instruction adalah load
static bool is_load_instruction(ccvm_instruction_t* inst) {
    return inst && ccvm_instruction_get_opcode(inst) == CCVM_OP_LOAD;
}

// Fungsi untuk mengecek apakah instruction adalah store
static bool is_store_instruction(ccvm_instruction_t* inst) {
    return inst && ccvm_instruction_get_opcode(inst) == CCVM_OP_STORE;
}

// Fungsi untuk mengecek apakah instruction adalah phi
static bool is_phi_instruction(ccvm_instruction_t* inst) {
    return inst && ccvm_instruction_get_opcode(inst) == CCVM_OP_PHI;
}

// Fungsi untuk mengecek apakah alloca dapat dipromosikan
static bool can_promote_alloca(ccvm_instruction_t* alloca_inst, mem2reg_context_t* ctx) {
    // Allocas hanya dapat dipromosikan jika:
    // 1. Ukuran tetap dan kecil
    // 2. Hanya digunakan oleh load/store dalam function yang sama
    // 3. Tidak digunakan sebagai pointer yang dilewatkan ke function lain
    // 4. Tidak digunakan untuk operasi pointer arithmetic
    
    ccvm_type_t* alloca_type = ccvm_instruction_get_type(alloca_inst);
    if (!alloca_type) return false;
    
    // TODO: Implementasi pengecekan yang lebih comprehensive
    return true;
}

// Fungsi untuk mengumpulkan semua allocas dalam function
static void collect_allocas(ccvm_function_t* function, mem2reg_context_t* ctx) {
    size_t bb_count = ccvm_function_get_basic_block_count(function);
    
    for (size_t i = 0; i < bb_count; i++) {
        ccvm_basic_block_t* bb = ccvm_function_get_basic_block(function, i);
        size_t inst_count = ccvm_basic_block_get_instruction_count(bb);
        
        for (size_t j = 0; j < inst_count; j++) {
            ccvm_instruction_t* inst = ccvm_basic_block_get_instruction(bb, j);
            
            if (is_alloca_instruction(inst)) {
                ctx->alloca_count++;
                
                if (can_promote_alloca(inst, ctx)) {
                    hash_set_add(ctx->promoted_allocas, inst);
                }
            }
        }
    }
}

// Fungsi untuk menemukan definitions dan uses dari alloca
static void find_definitions_and_uses(ccvm_instruction_t* alloca_inst, mem2reg_context_t* ctx) {
    // TODO: Implementasi traversing CFG untuk menemukan semua load/store
    // yang menggunakan alloca ini
}

// Fungsi untuk membuat phi nodes
static void place_phi_nodes(mem2reg_context_t* ctx) {
    // Algoritma minimal SSA:
    // 1. Untuk setiap alloca yang dipromosikan
    // 2. Temukan basic blocks yang perlu phi nodes
    // 3. Buat phi nodes di blocks tersebut
    
    // TODO: Implementasi algoritma phi placement yang efisien
    // (e.g., menggunakan dominator frontier)
}

// Fungsi untuk mengganti alloca dengan register references
static void rename_variables(mem2reg_context_t* ctx) {
    // TODO: Implementasi variable renaming
    // 1. Ganti load instructions dengan direct register references
    // 2. Ganti store instructions dengan register assignments
    // 3. Update phi nodes dengan operands yang benar
}

// Fungsi untuk membersihkan allocas yang sudah tidak digunakan
static void remove_dead_allocas(mem2reg_context_t* ctx) {
    size_t eliminated = 0;
    
    // TODO: Implementasi penghapusan allocas yang sudah dipromosikan
    // dan semua load/store yang menggunakannya
    
    ctx->eliminated_count = eliminated;
}

// Main Mem2Reg pass function
static enum ccvm_pass_result mem2reg_run(ccvm_pass_t* pass, void* ir_unit) {
    if (!pass || !ir_unit) {
        return CCVM_PASS_INVALID_INPUT;
    }
    
    ccvm_function_t* function = (ccvm_function_t*)ir_unit;
    
    // Initialize context
    mem2reg_context_t ctx = {0};
    ctx.function = function;
    ctx.alloca_map = hash_map_create(16);
    ctx.def_map = hash_map_create(16);
    ctx.use_map = hash_map_create(16);
    ctx.promoted_allocas = hash_set_create(16);
    
    if (!ctx.alloca_map || !ctx.def_map || !ctx.use_map || !ctx.promoted_allocas) {
        hash_map_destroy(ctx.alloca_map);
        hash_map_destroy(ctx.def_map);
        hash_map_destroy(ctx.use_map);
        hash_set_destroy(ctx.promoted_allocas);
        return CCVM_PASS_ERROR;
    }
    
    // Step 1: Kumpulkan semua allocas
    collect_allocas(function, &ctx);
    
    // Step 2: Analisis definitions dan uses
    // TODO: Implementasi comprehensive analysis
    
    // Step 3: Place phi nodes
    place_phi_nodes(&ctx);
    
    // Step 4: Rename variables
    rename_variables(&ctx);
    
    // Step 5: Remove dead allocas
    remove_dead_allocas(&ctx);
    
    // Store statistics
    pass->private_data = malloc(sizeof(mem2reg_context_t));
    if (pass->private_data) {
        memcpy(pass->private_data, &ctx, sizeof(mem2reg_context_t));
    }
    
    // Cleanup temporary data structures
    hash_map_destroy(ctx.alloca_map);
    hash_map_destroy(ctx.def_map);
    hash_map_destroy(ctx.use_map);
    hash_set_destroy(ctx.promoted_allocas);
    
    // Return success jika ada perubahan
    return ctx.eliminated_count > 0 ? CCVM_PASS_SUCCESS : CCVM_PASS_NO_CHANGE;
}

static void mem2reg_initialize(ccvm_pass_t* pass) {
    // No initialization needed
}

static void mem2reg_finalize(ccvm_pass_t* pass) {
    if (pass && pass->private_data) {
        free(pass->private_data);
        pass->private_data = NULL;
    }
}

static void mem2reg_destroy(ccvm_pass_t* pass) {
    mem2reg_finalize(pass);
}

static bool mem2reg_is_required(ccvm_pass_t* pass, const void* ir_unit) {
    if (!pass || !ir_unit) {
        return false;
    }
    
    ccvm_function_t* function = (ccvm_function_t*)ir_unit;
    
    // Check if function contains any allocas
    size_t bb_count = ccvm_function_get_basic_block_count(function);
    
    for (size_t i = 0; i < bb_count; i++) {
        ccvm_basic_block_t* bb = ccvm_function_get_basic_block(function, i);
        size_t inst_count = ccvm_basic_block_get_instruction_count(bb);
        
        for (size_t j = 0; j < inst_count; j++) {
            ccvm_instruction_t* inst = ccvm_basic_block_get_instruction(bb, j);
            if (is_alloca_instruction(inst)) {
                return true;
            }
        }
    }
    
    return false;
}

static void mem2reg_get_statistics(ccvm_pass_t* pass, void* stats) {
    if (!pass || !pass->private_data || !stats) {
        return;
    }
    
    mem2reg_context_t* ctx = (mem2reg_context_t*)pass->private_data;
    
    // TODO: Copy statistics to output structure
    (void)ctx; // Suppress unused parameter warning
    (void)stats; // Suppress unused parameter warning
}

// Pass vtable
static const struct ccvm_pass_vtable mem2reg_vtable = {
    .run = mem2reg_run,
    .initialize = mem2reg_initialize,
    .finalize = mem2reg_finalize,
    .destroy = mem2reg_destroy,
    .is_required = mem2reg_is_required,
    .get_statistics = mem2reg_get_statistics,
};

// Pass info
static const struct ccvm_pass_info mem2reg_info = {
    .name = "mem2reg",
    .description = "Promote memory references to be register references",
    .type = CCVM_PASS_FUNCTION,
    .strategy = CCVM_PASS_ONCE,
    .is_analysis_pass = false,
    .preserves_all = false,
    .modifies_cfg = true,
    .dependency_count = 2,
    .dependencies = (struct ccvm_pass_dependency[]){
        { .pass_name = "dom_tree", .required = true },
        { .pass_name = "cfg_simplification", .required = false },
    },
};

// Create Mem2Reg pass
ccvm_pass_t* ccvm_create_mem2reg_pass(void) {
    ccvm_pass_t* pass = (ccvm_pass_t*)calloc(1, sizeof(ccvm_pass_t));
    if (!pass) {
        return NULL;
    }
    
    pass->vtable = &mem2reg_vtable;
    pass->info = &mem2reg_info;
    pass->is_enabled = true;
    pass->is_initialized = false;
    pass->execution_count = 0;
    pass->total_time_ms = 0;
    pass->last_result = CCVM_PASS_SUCCESS;
    
    return pass;
}