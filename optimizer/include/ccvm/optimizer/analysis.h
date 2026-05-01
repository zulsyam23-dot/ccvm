#ifndef CCVM_OPTIMIZER_ANALYSIS_H
#define CCVM_OPTIMIZER_ANALYSIS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ir_types.h"

// Analysis types
enum ccvm_analysis_type {
    CCVM_ANALYSIS_DOMINATOR_TREE,
    CCVM_ANALYSIS_POST_DOMINATOR_TREE,
    CCVM_ANALYSIS_DOMINANCE_FRONTIERS,
    CCVM_ANALYSIS_LOOP_INFO,
    CCVM_ANALYSIS_SCALAR_EVOLUTION,
    CCVM_ANALYSIS_ALIAS_ANALYSIS,
    CCVM_ANALYSIS_LIVENESS,
    CCVM_ANALYSIS_REACHING_DEFINITIONS,
    CCVM_ANALYSIS_AVAILABLE_EXPRESSIONS,
    CCVM_ANALYSIS_CONSTANT_RANGE,
    CCVM_ANALYSIS_VALUE_TRACKING,
    CCVM_ANALYSIS_CALL_GRAPH,
    CCVM_ANALYSIS_CONTROL_DEPENDENCE,
    CCVM_ANALYSIS_DATA_DEPENDENCE,
    CCVM_ANALYSIS_MEMORY_DEPENDENCE,
};

// Analysis result codes
enum ccvm_analysis_result {
    CCVM_ANALYSIS_SUCCESS = 0,
    CCVM_ANALYSIS_INVALID_INPUT = -1,
    CCVM_ANALYSIS_OUT_OF_MEMORY = -2,
    CCVM_ANALYSIS_TIMEOUT = -3,
    CCVM_ANALYSIS_INCONSISTENT = -4,
    CCVM_ANALYSIS_UNSUPPORTED = -5,
};

// Forward declarations for analysis structures
typedef struct ccvm_dominator_tree ccvm_dominator_tree_t;
typedef struct ccvm_loop_info ccvm_loop_info_t;
typedef struct ccvm_alias_analysis ccvm_alias_analysis_t;
typedef struct ccvm_scalar_evolution ccvm_scalar_evolution_t;
typedef struct ccvm_liveness_analysis ccvm_liveness_analysis_t;
typedef struct ccvm_call_graph ccvm_call_graph_t;

// Dominator tree analysis
ccvm_dominator_tree_t* ccvm_analyze_dominator_tree(ccvm_function_t* function);
ccvm_dominator_tree_t* ccvm_analyze_post_dominator_tree(ccvm_function_t* function);
void ccvm_dominator_tree_destroy(ccvm_dominator_tree_t* tree);

// Loop analysis
ccvm_loop_info_t* ccvm_analyze_loops(ccvm_function_t* function);
void ccvm_loop_info_destroy(ccvm_loop_info_t* loop_info);

// Alias analysis
ccvm_alias_analysis_t* ccvm_analyze_alias(ccvm_function_t* function);
void ccvm_alias_analysis_destroy(ccvm_alias_analysis_t* alias_analysis);

// Scalar evolution analysis
ccvm_scalar_evolution_t* ccvm_analyze_scalar_evolution(ccvm_function_t* function);
void ccvm_scalar_evolution_destroy(ccvm_scalar_evolution_t* se);

// Liveness analysis
ccvm_liveness_analysis_t* ccvm_analyze_liveness(ccvm_function_t* function);
void ccvm_liveness_analysis_destroy(ccvm_liveness_analysis_t* liveness);

// Call graph analysis
ccvm_call_graph_t* ccvm_analyze_call_graph(ccvm_module_t* module);
void ccvm_call_graph_destroy(ccvm_call_graph_t* call_graph);

// Analysis caching
void ccvm_analysis_cache_clear(void);
void ccvm_analysis_cache_invalidate(ccvm_function_t* function);

#ifdef __cplusplus
}
#endif

#endif // CCVM_OPTIMIZER_ANALYSIS_H