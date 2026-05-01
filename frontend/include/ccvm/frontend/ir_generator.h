#ifndef CCVM_FRONTEND_IR_GENERATOR_H
#define CCVM_FRONTEND_IR_GENERATOR_H

#include "ast.h"
#include <string>
#include <vector>
#include <map>

namespace ccvm {
namespace frontend {

struct IRFunction {
    String name;
    String return_type;
    struct Parameter {
        String name;
        String type;
    };
    Vector<Parameter> parameters;
    Vector<String> body;
};

struct IRModule {
    String module_name;
    Vector<IRFunction> functions;
    Vector<String> globals;
    
    String to_string() const;
};

class IRGenerator {
private:
    IRModule module_;
    int temp_counter_;
    String current_function_;
    Vector<String> current_block_;
    Vector<String> alloca_vars_;
    
    String new_temp();
    String generate_expression(Ptr<Expression> expr);
    void generate_statement(Ptr<ASTNode> stmt);
    void generate_function_decl(Ptr<FunctionDeclaration> decl);
    void generate_function_def(Ptr<FunctionDefinition> def);
    void generate_variable_decl(Ptr<VariableDeclaration> decl);
    
public:
    IRGenerator() : temp_counter_(0) {}
    
    void generate(ASTNode& node);
    String get_ir() const;
    const IRModule& get_module() const { return module_; }
};

} // namespace frontend
} // namespace ccvm

#endif
