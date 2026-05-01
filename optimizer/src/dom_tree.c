/**
 * @file dom_tree.c
 * @brief Dominator Tree construction and analysis
 * @author CCVM Team
 * @date 2026
 */

#include "ccvm/optimizer/dom_tree.h"
#include <stdlib.h>

struct dom_tree* ccvm_dom_tree_create() {
    return (struct dom_tree*)calloc(1, sizeof(struct dom_tree));
}

void ccvm_dom_tree_destroy(struct dom_tree* tree) {
    if (tree) free(tree);
}
