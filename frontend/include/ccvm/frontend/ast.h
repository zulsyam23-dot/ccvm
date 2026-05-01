#ifndef CCVM_FRONTEND_AST_H
#define CCVM_FRONTEND_AST_H

#include "common.h"
#include "token.h"
#include <memory>
#include <vector>

namespace ccvm {
namespace frontend {

class ASTVisitor;

class ASTNode {
public:
    enum class NodeType {
        TRANSLATION_UNIT,
        FUNCTION_DECLARATION,
        FUNCTION_DEFINITION,
        VARIABLE_DECLARATION,
        TYPE_DECLARATION,
        COMPOUND_STATEMENT,
        EXPRESSION_STATEMENT,
        IF_STATEMENT,
        WHILE_STATEMENT,
        FOR_STATEMENT,
        DO_WHILE_STATEMENT,
        SWITCH_STATEMENT,
        CASE_STATEMENT,
        DEFAULT_STATEMENT,
        BREAK_STATEMENT,
        CONTINUE_STATEMENT,
        RETURN_STATEMENT,
        GOTO_STATEMENT,
        LABEL_STATEMENT,
        BINARY_EXPRESSION,
        UNARY_EXPRESSION,
        CALL_EXPRESSION,
        MEMBER_EXPRESSION,
        ARRAY_SUBSCRIPT_EXPRESSION,
        CONDITIONAL_EXPRESSION,
        CAST_EXPRESSION,
        SIZEOF_EXPRESSION,
        ALIGNOF_EXPRESSION,
        INTEGER_LITERAL,
        FLOAT_LITERAL,
        STRING_LITERAL,
        CHARACTER_LITERAL,
        BOOLEAN_LITERAL,
        NULL_LITERAL,
        IDENTIFIER,
        QUALIFIED_IDENTIFIER,
        PRIMITIVE_TYPE,
        POINTER_TYPE,
        REFERENCE_TYPE,
        ARRAY_TYPE,
        FUNCTION_TYPE,
        STRUCT_TYPE,
        UNION_TYPE,
        ENUM_TYPE,
        TYPEDEF_TYPE,
        PARAMETER_DECLARATION,
        FIELD_DECLARATION,
        ENUM_CONSTANT,
        INITIALIZER_LIST,
        DESIGNATED_INITIALIZER,
        PREPROCESSOR_INCLUDE,
        PREPROCESSOR_DEFINE,
        PREPROCESSOR_IFDEF,
        PREPROCESSOR_IFNDEF,
        PREPROCESSOR_ENDIF,
        LIFETIME,
        BORROW_EXPRESSION,
        MATCH_EXPRESSION,
        PATTERN,
        TRAIT_DECLARATION,
        IMPL_DECLARATION,
        MACRO_CALL,
        QUOTE_EXPRESSION,
        MODULE_DECLARATION,
        TEMPLATE_DECLARATION,
        TEMPLATE_PARAMETER,
        TEMPLATE_ARGUMENT,
        ATTRIBUTE,
        COMMENT,
        UNKNOWN
    };

protected:
    NodeType node_type_;
    SourceRange source_range_;
    Vector<Ptr<ASTNode>> children_;
    
public:
    explicit ASTNode(NodeType type) : node_type_(type) {}
    virtual ~ASTNode() = default;
    
    NodeType getNodeType() const { return node_type_; }
    String getNodeTypeName() const;
    
    const SourceRange& getSourceRange() const { return source_range_; }
    void setSourceRange(const SourceRange& range) { source_range_ = range; }
    
    const Vector<Ptr<ASTNode>>& getChildren() const { return children_; }
    Vector<Ptr<ASTNode>>& getChildren() { return children_; }
    void addChild(Ptr<ASTNode> child) { children_.push_back(std::move(child)); }
    void setChildren(Vector<Ptr<ASTNode>> children) { children_ = std::move(children); }
    
    virtual void accept(ASTVisitor& visitor) = 0;
    virtual String toString() const = 0;
    bool isStatement() const;
    bool isExpression() const;
    bool isDeclaration() const;
    bool isType() const;
};

class TypeNode : public ASTNode {
public:
    explicit TypeNode(NodeType type) : ASTNode(type) {}
    virtual String getTypeName() const = 0;
};

class Expression : public ASTNode {
public:
    explicit Expression(NodeType type) : ASTNode(type) {}
};

class Statement : public ASTNode {
public:
    explicit Statement(NodeType type) : ASTNode(type) {}
};

class Declaration : public ASTNode {
public:
    explicit Declaration(NodeType type) : ASTNode(type) {}
    virtual String getName() const = 0;
};

class TranslationUnit : public ASTNode {
private:
    Vector<Ptr<ASTNode>> declarations_;
    
public:
    TranslationUnit() : ASTNode(NodeType::TRANSLATION_UNIT) {}
    
    void addDeclaration(Ptr<ASTNode> decl) { declarations_.push_back(std::move(decl)); }
    const Vector<Ptr<ASTNode>>& getDeclarations() const { return declarations_; }
    
    void accept(ASTVisitor& visitor) override;
    String toString() const override { return "TranslationUnit"; }
};

class FunctionDeclaration : public Declaration {
private:
    String name_;
    Ptr<TypeNode> return_type_;
    Vector<Ptr<Declaration>> parameters_;
    bool is_variadic_;
    Vector<String> specifiers_;
    
public:
    FunctionDeclaration(StringRef name, Ptr<TypeNode> returnType)
        : Declaration(NodeType::FUNCTION_DECLARATION), name_(name), 
          return_type_(std::move(returnType)), is_variadic_(false) {}
    
    String getName() const override { return name_; }
    const Ptr<TypeNode>& getReturnType() const { return return_type_; }
    const Vector<Ptr<Declaration>>& getParameters() const { return parameters_; }
    Vector<Ptr<Declaration>>& getParameters() { return parameters_; }
    bool isVariadic() const { return is_variadic_; }
    void setVariadic(bool variadic) { is_variadic_ = variadic; }
    
    void accept(ASTVisitor& visitor) override;
    String toString() const override;
};

class FunctionDefinition : public FunctionDeclaration {
private:
    Ptr<Statement> body_;
    
public:
    FunctionDefinition(StringRef name, Ptr<TypeNode> returnType, Ptr<Statement> body)
        : FunctionDeclaration(name, std::move(returnType)), 
          body_(std::move(body)) {
        node_type_ = NodeType::FUNCTION_DEFINITION;
    }
    
    const Ptr<Statement>& getBody() const { return body_; }
    void setBody(Ptr<Statement> body) { body_ = std::move(body); }
    
    void accept(ASTVisitor& visitor) override;
    String toString() const override;
};

class VariableDeclaration : public Declaration {
private:
    String name_;
    Ptr<TypeNode> type_;
    Ptr<Expression> initializer_;
    Vector<String> specifiers_;
    
public:
    VariableDeclaration(StringRef name, Ptr<TypeNode> type, Ptr<Expression> init = nullptr)
        : Declaration(NodeType::VARIABLE_DECLARATION), name_(name), 
          type_(std::move(type)), initializer_(std::move(init)) {}
    
    String getName() const override { return name_; }
    const Ptr<TypeNode>& getType() const { return type_; }
    const Ptr<Expression>& getInitializer() const { return initializer_; }
    void setInitializer(Ptr<Expression> init) { initializer_ = std::move(init); }
    
    void accept(ASTVisitor& visitor) override;
    String toString() const override;
};

class CompoundStatement : public Statement {
private:
    Vector<Ptr<Statement>> statements_;
    
public:
    CompoundStatement() : Statement(NodeType::COMPOUND_STATEMENT) {}
    
    const Vector<Ptr<Statement>>& getStatements() const { return statements_; }
    Vector<Ptr<Statement>>& getStatements() { return statements_; }
    void addStatement(Ptr<Statement> stmt) { statements_.push_back(std::move(stmt)); }
    
    void accept(ASTVisitor& visitor) override;
    String toString() const override;
};

class ExpressionStatement : public Statement {
private:
    Ptr<Expression> expression_;
    
public:
    explicit ExpressionStatement(Ptr<Expression> expr)
        : Statement(NodeType::EXPRESSION_STATEMENT), expression_(std::move(expr)) {}
    
    const Ptr<Expression>& getExpression() const { return expression_; }
    void setExpression(Ptr<Expression> expr) { expression_ = std::move(expr); }
    
    void accept(ASTVisitor& visitor) override;
    String toString() const override;
};

class IfStatement : public Statement {
private:
    Ptr<Expression> condition_;
    Ptr<Statement> then_statement_;
    Ptr<Statement> else_statement_;
    
public:
    IfStatement(Ptr<Expression> cond, Ptr<Statement> thenStmt, Ptr<Statement> elseStmt = nullptr)
        : Statement(NodeType::IF_STATEMENT), condition_(std::move(cond)), 
          then_statement_(std::move(thenStmt)), else_statement_(std::move(elseStmt)) {}
    
    const Ptr<Expression>& getCondition() const { return condition_; }
    const Ptr<Statement>& getThenStatement() const { return then_statement_; }
    const Ptr<Statement>& getElseStatement() const { return else_statement_; }
    bool hasElseStatement() const { return else_statement_ != nullptr; }
    
    void accept(ASTVisitor& visitor) override;
    String toString() const override;
};

class WhileStatement : public Statement {
private:
    Ptr<Expression> condition_;
    Ptr<Statement> body_;
    
public:
    WhileStatement(Ptr<Expression> cond, Ptr<Statement> body)
        : Statement(NodeType::WHILE_STATEMENT), condition_(std::move(cond)), 
          body_(std::move(body)) {}
    
    const Ptr<Expression>& getCondition() const { return condition_; }
    const Ptr<Statement>& getBody() const { return body_; }
    
    void accept(ASTVisitor& visitor) override;
    String toString() const override;
};

class ForStatement : public Statement {
private:
    Ptr<Declaration> init_;
    Ptr<Expression> condition_;
    Ptr<Expression> increment_;
    Ptr<Statement> body_;
    
public:
    ForStatement(Ptr<Declaration> init, Ptr<Expression> cond, 
                 Ptr<Expression> inc, Ptr<Statement> body)
        : Statement(NodeType::FOR_STATEMENT), init_(std::move(init)), 
          condition_(std::move(cond)), increment_(std::move(inc)), 
          body_(std::move(body)) {}
    
    const Ptr<Declaration>& getInit() const { return init_; }
    const Ptr<Expression>& getCondition() const { return condition_; }
    const Ptr<Expression>& getIncrement() const { return increment_; }
    const Ptr<Statement>& getBody() const { return body_; }
    
    void accept(ASTVisitor& visitor) override;
    String toString() const override;
};

class ReturnStatement : public Statement {
private:
    Ptr<Expression> value_;
    
public:
    explicit ReturnStatement(Ptr<Expression> val = nullptr)
        : Statement(NodeType::RETURN_STATEMENT), value_(std::move(val)) {}
    
    const Ptr<Expression>& getValue() const { return value_; }
    bool hasValue() const { return value_ != nullptr; }
    
    void accept(ASTVisitor& visitor) override;
    String toString() const override;
};

class BinaryExpression : public Expression {
public:
    enum class Operator {
        ADD, SUB, MUL, DIV, MOD,
        EQ, NE, LT, GT, LE, GE,
        LOGICAL_AND, LOGICAL_OR,
        BITWISE_AND, BITWISE_OR, BITWISE_XOR,
        SHL, SHR,
        ASSIGN, ADD_ASSIGN, SUB_ASSIGN, MUL_ASSIGN, DIV_ASSIGN, MOD_ASSIGN
    };

private:
    Operator op_;
    Ptr<Expression> left_;
    Ptr<Expression> right_;
    
public:
    BinaryExpression(Operator op, Ptr<Expression> left, Ptr<Expression> right)
        : Expression(NodeType::BINARY_EXPRESSION), op_(op), 
          left_(std::move(left)), right_(std::move(right)) {}
    
    Operator getOperator() const { return op_; }
    const Ptr<Expression>& getLeft() const { return left_; }
    const Ptr<Expression>& getRight() const { return right_; }
    
    void accept(ASTVisitor& visitor) override;
    String toString() const override;
};

class UnaryExpression : public Expression {
public:
    enum class Operator {
        PLUS, MINUS, LOGICAL_NOT, BITWISE_NOT,
        PRE_INCREMENT, PRE_DECREMENT,
        POST_INCREMENT, POST_DECREMENT,
        DEREFERENCE, ADDRESS_OF
    };

private:
    Operator op_;
    Ptr<Expression> operand_;
    bool is_postfix_;
    
public:
    UnaryExpression(Operator op, Ptr<Expression> operand, bool isPostfix = false)
        : Expression(NodeType::UNARY_EXPRESSION), op_(op), 
          operand_(std::move(operand)), is_postfix_(isPostfix) {}
    
    Operator getOperator() const { return op_; }
    const Ptr<Expression>& getOperand() const { return operand_; }
    bool isPostfix() const { return is_postfix_; }
    
    void accept(ASTVisitor& visitor) override;
    String toString() const override;
};

class CallExpression : public Expression {
private:
    Ptr<Expression> callee_;
    Vector<Ptr<Expression>> arguments_;
    
public:
    CallExpression(Ptr<Expression> callee, Vector<Ptr<Expression>> args)
        : Expression(NodeType::CALL_EXPRESSION), callee_(std::move(callee)), 
          arguments_(std::move(args)) {}
    
    const Ptr<Expression>& getCallee() const { return callee_; }
    const Vector<Ptr<Expression>>& getArguments() const { return arguments_; }
    Vector<Ptr<Expression>>& getArguments() { return arguments_; }
    
    void accept(ASTVisitor& visitor) override;
    String toString() const override;
};

class IdentifierExpression : public Expression {
private:
    String name_;
    
public:
    explicit IdentifierExpression(StringRef name)
        : Expression(NodeType::IDENTIFIER), name_(name) {}
    
    const String& getName() const { return name_; }
    
    void accept(ASTVisitor& visitor) override;
    String toString() const override;
};

class IntegerLiteral : public Expression {
private:
    IntType value_;
    String suffix_;
    
public:
    IntegerLiteral(IntType value, StringRef suffix = "")
        : Expression(NodeType::INTEGER_LITERAL), value_(value), suffix_(suffix) {}
    
    IntType getValue() const { return value_; }
    const String& getSuffix() const { return suffix_; }
    
    void accept(ASTVisitor& visitor) override;
    String toString() const override;
};

class FloatLiteral : public Expression {
private:
    FloatType value_;
    String suffix_;
    
public:
    FloatLiteral(FloatType value, StringRef suffix = "")
        : Expression(NodeType::FLOAT_LITERAL), value_(value), suffix_(suffix) {}
    
    FloatType getValue() const { return value_; }
    const String& getSuffix() const { return suffix_; }
    
    void accept(ASTVisitor& visitor) override;
    String toString() const override;
};

class StringLiteral : public Expression {
private:
    String value_;
    bool is_wide_;
    bool is_raw_;
    
public:
    StringLiteral(StringRef value, bool wide = false, bool raw = false)
        : Expression(NodeType::STRING_LITERAL), value_(value), 
          is_wide_(wide), is_raw_(raw) {}
    
    const String& getValue() const { return value_; }
    bool isWide() const { return is_wide_; }
    bool isRaw() const { return is_raw_; }
    
    void accept(ASTVisitor& visitor) override;
    String toString() const override;
};

class PrimitiveType : public TypeNode {
public:
    enum class Kind {
        VOID, BOOL, CHAR, SHORT, INT, LONG, FLOAT, DOUBLE,
        SIGNED, UNSIGNED, WCHAR_T, CHAR16_T, CHAR32_T
    };

private:
    Kind kind_;
    bool is_signed_;
    
public:
    explicit PrimitiveType(Kind kind, bool isSigned = true)
        : TypeNode(NodeType::PRIMITIVE_TYPE), kind_(kind), is_signed_(isSigned) {}
    
    Kind getKind() const { return kind_; }
    bool isSigned() const { return is_signed_; }
    String getTypeName() const override;
    
    void accept(ASTVisitor& visitor) override;
    String toString() const override;
};

class PointerType : public TypeNode {
private:
    Ptr<TypeNode> pointee_type_;
    bool is_const_;
    bool is_volatile_;
    
public:
    PointerType(Ptr<TypeNode> pointee, bool isConst = false, bool isVolatile = false)
        : TypeNode(NodeType::POINTER_TYPE), pointee_type_(std::move(pointee)), 
          is_const_(isConst), is_volatile_(isVolatile) {}
    
    const Ptr<TypeNode>& getPointeeType() const { return pointee_type_; }
    bool isConst() const { return is_const_; }
    bool isVolatile() const { return is_volatile_; }
    String getTypeName() const override;
    
    void accept(ASTVisitor& visitor) override;
    String toString() const override;
};

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    
    virtual void visitTranslationUnit(TranslationUnit& node) = 0;
    virtual void visitFunctionDeclaration(FunctionDeclaration& node) = 0;
    virtual void visitFunctionDefinition(FunctionDefinition& node) = 0;
    virtual void visitVariableDeclaration(VariableDeclaration& node) = 0;
    virtual void visitCompoundStatement(CompoundStatement& node) = 0;
    virtual void visitExpressionStatement(ExpressionStatement& node) = 0;
    virtual void visitIfStatement(IfStatement& node) = 0;
    virtual void visitWhileStatement(WhileStatement& node) = 0;
    virtual void visitForStatement(ForStatement& node) = 0;
    virtual void visitReturnStatement(ReturnStatement& node) = 0;
    virtual void visitBinaryExpression(BinaryExpression& node) = 0;
    virtual void visitUnaryExpression(UnaryExpression& node) = 0;
    virtual void visitCallExpression(CallExpression& node) = 0;
    virtual void visitIdentifierExpression(IdentifierExpression& node) = 0;
    virtual void visitIntegerLiteral(IntegerLiteral& node) = 0;
    virtual void visitFloatLiteral(FloatLiteral& node) = 0;
    virtual void visitStringLiteral(StringLiteral& node) = 0;
    virtual void visitPrimitiveType(PrimitiveType& node) = 0;
    virtual void visitPointerType(PointerType& node) = 0;
};

class ASTBuilder {
public:
    static Ptr<TranslationUnit> createTranslationUnit();
    static Ptr<FunctionDeclaration> createFunctionDeclaration(StringRef name, Ptr<TypeNode> returnType);
    static Ptr<FunctionDefinition> createFunctionDefinition(StringRef name, Ptr<TypeNode> returnType, Ptr<Statement> body);
    static Ptr<VariableDeclaration> createVariableDeclaration(StringRef name, Ptr<TypeNode> type, Ptr<Expression> init = nullptr);
    static Ptr<CompoundStatement> createCompoundStatement();
    static Ptr<ExpressionStatement> createExpressionStatement(Ptr<Expression> expr);
    static Ptr<IfStatement> createIfStatement(Ptr<Expression> cond, Ptr<Statement> thenStmt, Ptr<Statement> elseStmt = nullptr);
    static Ptr<WhileStatement> createWhileStatement(Ptr<Expression> cond, Ptr<Statement> body);
    static Ptr<ForStatement> createForStatement(Ptr<Declaration> init, Ptr<Expression> cond, Ptr<Expression> inc, Ptr<Statement> body);
    static Ptr<ReturnStatement> createReturnStatement(Ptr<Expression> value = nullptr);
    static Ptr<BinaryExpression> createBinaryExpression(BinaryExpression::Operator op, Ptr<Expression> left, Ptr<Expression> right);
    static Ptr<UnaryExpression> createUnaryExpression(UnaryExpression::Operator op, Ptr<Expression> operand, bool isPostfix = false);
    static Ptr<CallExpression> createCallExpression(Ptr<Expression> callee, Vector<Ptr<Expression>> args);
    static Ptr<IdentifierExpression> createIdentifierExpression(StringRef name);
    static Ptr<IntegerLiteral> createIntegerLiteral(IntType value, StringRef suffix = "");
    static Ptr<FloatLiteral> createFloatLiteral(FloatType value, StringRef suffix = "");
    static Ptr<StringLiteral> createStringLiteral(StringRef value, bool wide = false, bool raw = false);
    static Ptr<PrimitiveType> createPrimitiveType(PrimitiveType::Kind kind, bool isSigned = true);
    static Ptr<PointerType> createPointerType(Ptr<TypeNode> pointee, bool isConst = false, bool isVolatile = false);
};

} // namespace frontend
} // namespace ccvm

#endif // CCVM_FRONTEND_AST_H
