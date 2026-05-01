#ifndef CCVM_OPTIMIZER_DOM_TREE_H
#define CCVM_OPTIMIZER_DOM_TREE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ir_types.h"
#include <stdbool.h>
#include <stddef.h>

// Dominator tree node
typedef struct ccvm_dom_tree_node {
    ccvm_basic_block_t* block;
    struct ccvm_dom_tree_node* parent;
    struct ccvm_dom_tree_node** children;
    size_t child_count;
    size_t depth;
    size_t dfs_num;
    size_t dfs_last;
} ccvm_dom_tree_node_t;

// Dominator tree
typedef struct ccvm_dom_tree {
    ccvm_function_t* function;
    ccvm_dom_tree_node_t* root;
    ccvm_dom_tree_node_t** nodes;
    size_t node_count;
    bool is_post_dom;
} ccvm_dom_tree_t;

// Create dominator tree
ccvm_dom_tree_t* ccvm_create_dominator_tree(ccvm_function_t* function, bool is_post_dom);

// Destroy dominator tree
void ccvm_destroy_dominator_tree(ccvm_dom_tree_t* tree);

// Dominator queries
bool ccvm_dom_tree_dominates(const ccvm_dom_tree_t* tree, 
                              ccvm_basic_block_t* a, 
                              ccvm_basic_block_t* b);

bool ccvm_dom_tree_strictly_dominates(const ccvm_dom_tree_t* tree, 
                                       ccvm_basic_block_t* a, 
                                       ccvm_basic_block_t* b);

ccvm_basic_block_t* ccvm_dom_tree_get_idom(const ccvm_dom_tree_t* tree, 
                                           ccvm_basic_block_t* block);

// Dominance frontier
ccvm_basic_block_t** ccvm_dom_tree_get_dominance_frontier(const ccvm_dom_tree_t* tree, 
                                                            ccvm_basic_block_t* block, 
                                                            size_t* count);

// Tree traversal
void ccvm_dom_tree_dfs(const ccvm_dom_tree_t* tree, 
                       void (*visit)(ccvm_dom_tree_node_t* node, void* data), 
                       void* data);

void ccvm_dom_tree_bfs(const ccvm_dom_tree_t* tree, 
                       void (*visit)(ccvm_dom_tree_node_t* node, void* data), 
                       void* data);

// Utility functions
size_t ccvm_dom_tree_get_depth(const ccvm_dom_tree_t* tree, ccvm_basic_block_t* block);
size_t ccvm_dom_tree_get_level(const ccvm_dom_tree_t* tree, ccvm_basic_block_t* block);
ccvm_dom_tree_node_t* ccvm_dom_tree_get_node(const ccvm_dom_tree_t* tree, ccvm_basic_block_t* block);

// Advanced dominator analysis
bool ccvm_dom_tree_is_reachable(const ccvm_dom_tree_t* tree, ccvm_basic_block_t* block);
bool ccvm_dom_tree_is_back_edge(const ccvm_dom_tree_t* tree, 
                                 ccvm_basic_block_t* from, 
                                 ccvm_basic_block_t* to);

// Loop detection using dominators
bool ccvm_dom_tree_is_loop_header(const ccvm_dom_tree_t* tree, ccvm_basic_block_t* block);
ccvm_basic_block_t** ccvm_dom_tree_get_loop_blocks(const ccvm_dom_tree_t* tree, 
                                                     ccvm_basic_block_t* header, 
                                                     size_t* count);

#ifdef __cplusplus
}
#endif

#endif // CCVM_OPTIMIZER_DOM_TREE_H