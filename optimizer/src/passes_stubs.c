#include "ccvm/optimizer/passes.h"
#include "ccvm/optimizer/pass_manager.h"
#include <stdlib.h>
#include <string.h>

ccvm_pass_manager_t* ccvm_create_pipeline(enum ccvm_pipeline_type type) {
    (void)type;
    return ccvm_pass_manager_create(NULL);
}

ccvm_pass_manager_t* ccvm_create_custom_pipeline(
    const char** pass_names,
    size_t pass_count,
    enum ccvm_pass_strategy strategy) {
    (void)pass_names; (void)pass_count; (void)strategy;
    return ccvm_pass_manager_create(NULL);
}

ccvm_pass_manager_t* ccvm_create_pipeline_with_config(const struct ccvm_pipeline_config* config) {
    if (!config) return ccvm_pass_manager_create(NULL);
    return ccvm_create_pipeline(config->type);
}

static const char* o1_seq[] = { CCVM_PASS_DCE, CCVM_PASS_CONST_FOLDING, NULL };
static const char* o2_seq[] = { CCVM_PASS_DCE, CCVM_PASS_CONST_FOLDING, CCVM_PASS_INLINING, NULL };
static const char* o3_seq[] = { CCVM_PASS_DCE, CCVM_PASS_CONST_FOLDING, CCVM_PASS_INLINING, CCVM_PASS_LOOP_OPT, NULL };
static const char* os_seq[] = { CCVM_PASS_DCE, CCVM_PASS_CONST_FOLDING, NULL };
static const char* oz_seq[] = { CCVM_PASS_DCE, NULL };

const char** ccvm_get_o1_pass_sequence(size_t* count) { if (count) *count = 2; return o1_seq; }
const char** ccvm_get_o2_pass_sequence(size_t* count) { if (count) *count = 3; return o2_seq; }
const char** ccvm_get_o3_pass_sequence(size_t* count) { if (count) *count = 4; return o3_seq; }
const char** ccvm_get_os_pass_sequence(size_t* count) { if (count) *count = 2; return os_seq; }
const char** ccvm_get_oz_pass_sequence(size_t* count) { if (count) *count = 1; return oz_seq; }

bool ccvm_validate_pass_order(const char** passes, size_t pass_count, char** error_message) {
    (void)passes; (void)pass_count; (void)error_message; return true;
}

enum ccvm_opt_result ccvm_resolve_pass_dependencies(
    const char** passes,
    size_t pass_count,
    const char*** resolved_passes,
    size_t* resolved_count) {
    if (resolved_passes) *resolved_passes = passes;
    if (resolved_count) *resolved_count = pass_count;
    (void)passes; (void)pass_count;
    return CCVM_OPT_SUCCESS;
}

void ccvm_get_pipeline_stats(const ccvm_pass_manager_t* pipeline, struct ccvm_pipeline_stats* stats) {
    if (!stats) return;
    memset(stats, 0, sizeof(*stats));
    if (pipeline) stats->total_passes = 0;
}

enum ccvm_opt_result ccvm_optimize_pipeline(ccvm_pass_manager_t* pipeline, enum ccvm_pipeline_type target_type) {
    (void)pipeline; (void)target_type; return CCVM_OPT_SUCCESS;
}

enum ccvm_opt_result ccvm_optimize_pipeline_with_profile(ccvm_pass_manager_t* pipeline, const char* profile_data, size_t profile_size) {
    (void)pipeline; (void)profile_data; (void)profile_size; return CCVM_OPT_SUCCESS;
}

int ccvm_compare_pipelines(const ccvm_pass_manager_t* pipeline1, const ccvm_pass_manager_t* pipeline2, struct ccvm_pipeline_stats* stats1, struct ccvm_pipeline_stats* stats2) {
    (void)pipeline1; (void)pipeline2; (void)stats1; (void)stats2; return 0;
}

enum ccvm_opt_result ccvm_serialize_pipeline(const ccvm_pass_manager_t* pipeline, char** serialized_data, size_t* serialized_size) {
    if (serialized_data) *serialized_data = NULL;
    if (serialized_size) *serialized_size = 0;
    (void)pipeline; return CCVM_OPT_SUCCESS;
}

enum ccvm_opt_result ccvm_deserialize_pipeline(ccvm_pass_manager_t* pipeline, const char* serialized_data, size_t serialized_size) {
    (void)pipeline; (void)serialized_data; (void)serialized_size; return CCVM_OPT_SUCCESS;
}

const char* ccvm_pipeline_type_to_string(enum ccvm_pipeline_type type) {
    switch(type) {
        case CCVM_PIPELINE_O0: return "O0";
        case CCVM_PIPELINE_O1: return "O1";
        case CCVM_PIPELINE_O2: return "O2";
        case CCVM_PIPELINE_O3: return "O3";
        case CCVM_PIPELINE_Os: return "Os";
        case CCVM_PIPELINE_Oz: return "Oz";
        case CCVM_PIPELINE_FAST: return "FAST";
        case CCVM_PIPELINE_PIPE: return "PIPE";
        default: return "unknown";
    }
}

enum ccvm_pipeline_type ccvm_string_to_pipeline_type(const char* str) {
    if (!str) return CCVM_PIPELINE_O0;
    if (strcmp(str,"O1")==0) return CCVM_PIPELINE_O1;
    if (strcmp(str,"O2")==0) return CCVM_PIPELINE_O2;
    if (strcmp(str,"O3")==0) return CCVM_PIPELINE_O3;
    if (strcmp(str,"Os")==0) return CCVM_PIPELINE_Os;
    if (strcmp(str,"Oz")==0) return CCVM_PIPELINE_Oz;
    return CCVM_PIPELINE_O0;
}

bool ccvm_is_analysis_pass(const char* pass_name) {
    if (!pass_name) return false;
    return (strcmp(pass_name, CCVM_PASS_DOM_TREE)==0 || strcmp(pass_name, CCVM_PASS_ALIAS_ANALYSIS)==0 || strcmp(pass_name, CCVM_PASS_SCALAR_EVOLUTION)==0 || strcmp(pass_name, CCVM_PASS_VALUE_TRACKING)==0);
}

bool ccvm_is_transform_pass(const char* pass_name) {
    if (!pass_name) return false;
    return (strcmp(pass_name, CCVM_PASS_DCE)==0 || strcmp(pass_name, CCVM_PASS_INLINING)==0 || strcmp(pass_name, CCVM_PASS_MEM2REG)==0 || strcmp(pass_name, CCVM_PASS_CONST_FOLDING)==0);
}

bool ccvm_is_ip_pass(const char* pass_name) { (void)pass_name; return false; }
bool ccvm_is_loop_pass(const char* pass_name) { (void)pass_name; return false; }
bool ccvm_is_vectorization_pass(const char* pass_name) { (void)pass_name; return false; }

enum ccvm_pass_category ccvm_get_pass_category(const char* pass_name) {
    (void)pass_name; return CCVM_PASS_CAT_MISC;
}

const char** ccvm_get_passes_in_category(enum ccvm_pass_category category, size_t* count) {
    (void)category; if (count) *count = 0; return NULL;
}
