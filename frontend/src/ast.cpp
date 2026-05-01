#include "ccvm/frontend/ast.h"
#include <sstream>

namespace ccvm::frontend {

String ASTNode::getNodeTypeName() const {
    switch (node_type_) {
        case NodeType::TRANSLATION_UNIT: return "TranslationUnit";
        case NodeType::FUNCTION_DEFINITION: return "FunctionDefinition";
        case NodeType::VARIABLE_DECLARATION: return "VariableDeclaration";
        case NodeType::COMPOUND_STATEMENT: return "CompoundStatement";
        case NodeType::EXPRESSION_STATEMENT: return "ExpressionStatement";
        case NodeType::BINARY_EXPRESSION: return "BinaryExpression";
        case NodeType::INTEGER_LITERAL: return "IntegerLiteral";
        case NodeType::FLOAT_LITERAL: return "FloatLiteral";
        case NodeType::IDENTIFIER: return "Identifier";
        case NodeType::RETURN_STATEMENT: return "ReturnStatement";
        case NodeType::IF_STATEMENT: return "IfStatement";
        case NodeType::WHILE_STATEMENT: return "WhileStatement";
        default: return "Unknown";
    }
}

bool ASTNode::isStatement() const {
    switch (node_type_) {
        case NodeType::COMPOUND_STATEMENT:
        case NodeType::EXPRESSION_STATEMENT:
        case NodeType::IF_STATEMENT:
        case NodeType::WHILE_STATEMENT:
        case NodeType::RETURN_STATEMENT:
            return true;
        default:
            return false;
    }
}

bool ASTNode::isExpression() const {
    switch (node_type_) {
        case NodeType::BINARY_EXPRESSION:
        case NodeType::UNARY_EXPRESSION:
        case NodeType::CALL_EXPRESSION:
        case NodeType::INTEGER_LITERAL:
        case NodeType::FLOAT_LITERAL:
        case NodeType::IDENTIFIER:
            return true;
        default:
            return false;
    }
}

bool ASTNode::isDeclaration() const {
    switch (node_type_) {
        case NodeType::FUNCTION_DECLARATION:
        case NodeType::FUNCTION_DEFINITION:
        case NodeType::VARIABLE_DECLARATION:
            return true;
        default:
            return false;
    }
}

bool ASTNode::isType() const {
    switch (node_type_) {
        case NodeType::PRIMITIVE_TYPE:
        case NodeType::POINTER_TYPE:
        case NodeType::REFERENCE_TYPE:
        case NodeType::ARRAY_TYPE:
        case NodeType::FUNCTION_TYPE:
        case NodeType::STRUCT_TYPE:
            return true;
        default:
            return false;
    }
}

void TranslationUnit::accept(ASTVisitor& visitor) { visitor.visitTranslationUnit(*this); }
void FunctionDeclaration::accept(ASTVisitor& visitor) { visitor.visitFunctionDeclaration(*this); }
void FunctionDefinition::accept(ASTVisitor& visitor) { visitor.visitFunctionDefinition(*this); }
void VariableDeclaration::accept(ASTVisitor& visitor) { visitor.visitVariableDeclaration(*this); }
void CompoundStatement::accept(ASTVisitor& visitor) { visitor.visitCompoundStatement(*this); }
void ExpressionStatement::accept(ASTVisitor& visitor) { visitor.visitExpressionStatement(*this); }
void IfStatement::accept(ASTVisitor& visitor) { visitor.visitIfStatement(*this); }
void WhileStatement::accept(ASTVisitor& visitor) { visitor.visitWhileStatement(*this); }
void ForStatement::accept(ASTVisitor& visitor) { visitor.visitForStatement(*this); }
void ReturnStatement::accept(ASTVisitor& visitor) { visitor.visitReturnStatement(*this); }
void BinaryExpression::accept(ASTVisitor& visitor) { visitor.visitBinaryExpression(*this); }
void UnaryExpression::accept(ASTVisitor& visitor) { visitor.visitUnaryExpression(*this); }
void CallExpression::accept(ASTVisitor& visitor) { visitor.visitCallExpression(*this); }
void IdentifierExpression::accept(ASTVisitor& visitor) { visitor.visitIdentifierExpression(*this); }
void IntegerLiteral::accept(ASTVisitor& visitor) { visitor.visitIntegerLiteral(*this); }
void FloatLiteral::accept(ASTVisitor& visitor) { visitor.visitFloatLiteral(*this); }
void StringLiteral::accept(ASTVisitor& visitor) { visitor.visitStringLiteral(*this); }
void PrimitiveType::accept(ASTVisitor& visitor) { visitor.visitPrimitiveType(*this); }
void PointerType::accept(ASTVisitor& visitor) { visitor.visitPointerType(*this); }

String FunctionDeclaration::toString() const { return "FunctionDeclaration: " + name_; }
String FunctionDefinition::toString() const { return "FunctionDefinition: " + getName(); }
String VariableDeclaration::toString() const { return "VariableDeclaration: " + name_; }
String CompoundStatement::toString() const { return "CompoundStatement"; }
String ExpressionStatement::toString() const { return "ExpressionStatement"; }
String IfStatement::toString() const { return "IfStatement"; }
String WhileStatement::toString() const { return "WhileStatement"; }
String ForStatement::toString() const { return "ForStatement"; }
String ReturnStatement::toString() const { return "ReturnStatement"; }
String BinaryExpression::toString() const { return "BinaryExpression"; }
String UnaryExpression::toString() const { return "UnaryExpression"; }
String CallExpression::toString() const { return "CallExpression"; }
String IdentifierExpression::toString() const { return "IdentifierExpression: " + name_; }
String IntegerLiteral::toString() const { return "IntegerLiteral: " + std::to_string(value_); }
String FloatLiteral::toString() const { return "FloatLiteral: " + std::to_string(value_); }
String StringLiteral::toString() const { return "StringLiteral: " + value_; }
String PrimitiveType::toString() const { return "PrimitiveType"; }
String PointerType::toString() const { return "PointerType"; }

String PrimitiveType::getTypeName() const {
    switch (kind_) {
        case Kind::VOID: return "void";
        case Kind::BOOL: return "bool";
        case Kind::CHAR: return "char";
        case Kind::SHORT: return "short";
        case Kind::INT: return "int";
        case Kind::LONG: return "long";
        case Kind::FLOAT: return "float";
        case Kind::DOUBLE: return "double";
        default: return "unknown";
    }
}

String PointerType::getTypeName() const {
    return pointee_type_->getTypeName() + "*";
}

Ptr<TranslationUnit> ASTBuilder::createTranslationUnit() {
    return std::make_shared<TranslationUnit>();
}

Ptr<FunctionDeclaration> ASTBuilder::createFunctionDeclaration(StringRef name, Ptr<TypeNode> returnType) {
    return std::make_shared<FunctionDeclaration>(name, returnType);
}

Ptr<FunctionDefinition> ASTBuilder::createFunctionDefinition(StringRef name, Ptr<TypeNode> returnType, Ptr<Statement> body) {
    return std::make_shared<FunctionDefinition>(name, returnType, body);
}

Ptr<VariableDeclaration> ASTBuilder::createVariableDeclaration(StringRef name, Ptr<TypeNode> type, Ptr<Expression> init) {
    return std::make_shared<VariableDeclaration>(name, type, init);
}

Ptr<CompoundStatement> ASTBuilder::createCompoundStatement() {
    return std::make_shared<CompoundStatement>();
}

Ptr<ExpressionStatement> ASTBuilder::createExpressionStatement(Ptr<Expression> expr) {
    return std::make_shared<ExpressionStatement>(expr);
}

Ptr<IfStatement> ASTBuilder::createIfStatement(Ptr<Expression> cond, Ptr<Statement> thenStmt, Ptr<Statement> elseStmt) {
    return std::make_shared<IfStatement>(cond, thenStmt, elseStmt);
}

Ptr<WhileStatement> ASTBuilder::createWhileStatement(Ptr<Expression> cond, Ptr<Statement> body) {
    return std::make_shared<WhileStatement>(cond, body);
}

Ptr<ForStatement> ASTBuilder::createForStatement(Ptr<Declaration> init, Ptr<Expression> cond, Ptr<Expression> inc, Ptr<Statement> body) {
    return std::make_shared<ForStatement>(init, cond, inc, body);
}

Ptr<ReturnStatement> ASTBuilder::createReturnStatement(Ptr<Expression> value) {
    return std::make_shared<ReturnStatement>(value);
}

Ptr<BinaryExpression> ASTBuilder::createBinaryExpression(BinaryExpression::Operator op, Ptr<Expression> left, Ptr<Expression> right) {
    return std::make_shared<BinaryExpression>(op, left, right);
}

Ptr<UnaryExpression> ASTBuilder::createUnaryExpression(UnaryExpression::Operator op, Ptr<Expression> operand, bool isPostfix) {
    return std::make_shared<UnaryExpression>(op, operand, isPostfix);
}

Ptr<CallExpression> ASTBuilder::createCallExpression(Ptr<Expression> callee, Vector<Ptr<Expression>> args) {
    return std::make_shared<CallExpression>(callee, args);
}

Ptr<IdentifierExpression> ASTBuilder::createIdentifierExpression(StringRef name) {
    return std::make_shared<IdentifierExpression>(name);
}

Ptr<IntegerLiteral> ASTBuilder::createIntegerLiteral(IntType value, StringRef suffix) {
    return std::make_shared<IntegerLiteral>(value, suffix);
}

Ptr<FloatLiteral> ASTBuilder::createFloatLiteral(FloatType value, StringRef suffix) {
    return std::make_shared<FloatLiteral>(value, suffix);
}

Ptr<StringLiteral> ASTBuilder::createStringLiteral(StringRef value, bool wide, bool raw) {
    return std::make_shared<StringLiteral>(value, wide, raw);
}

Ptr<PrimitiveType> ASTBuilder::createPrimitiveType(PrimitiveType::Kind kind, bool isSigned) {
    return std::make_shared<PrimitiveType>(kind, isSigned);
}

Ptr<PointerType> ASTBuilder::createPointerType(Ptr<TypeNode> pointee, bool isConst, bool isVolatile) {
    return std::make_shared<PointerType>(pointee, isConst, isVolatile);
}

} // namespace ccvm::frontend
