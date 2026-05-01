/**
 * @brief Semantic analyzer interface
 */

#ifndef CCVM_FRONTEND_SEMANTIC_ANALYZER_H
#define CCVM_FRONTEND_SEMANTIC_ANALYZER_H

#include "ast.h"

namespace ccvm {
namespace frontend {

class SemanticAnalyzer {
public:
    void analyze(ASTNode& node);
};

} // namespace frontend
} // namespace ccvm

#endif
