#ifndef CCVM_FRONTEND_PARSER_H
#define CCVM_FRONTEND_PARSER_H

#include "lexer.h"
#include "ast.h"
#include <memory>

namespace ccvm {
namespace frontend {

class Parser {
private:
    Vector<Token> tokens_;
    size_t current_token_idx_;
    
public:
    explicit Parser(Lexer& lexer);
    explicit Parser(Vector<Token> tokens);
    Ptr<ASTNode> parse();

private:
    Ptr<ASTNode> parseDeclaration();
    Ptr<ASTNode> parseFunctionDefinition(Token type, Token name);
    Ptr<ASTNode> parseVariableDeclaration(Token type, Token name);
    Ptr<ASTNode> parseCompoundStatement();
    Ptr<ASTNode> parseStatement();
    Ptr<ASTNode> parseExpressionStatement();
    Ptr<ASTNode> parseExpression();
    Ptr<ASTNode> parseAssignment();
    Ptr<ASTNode> parseBinary();
    Ptr<ASTNode> parseComparison();
    Ptr<ASTNode> parsePrimary();
    Ptr<ASTNode> parseIfStatement();
    Ptr<ASTNode> parseWhileStatement();
    Ptr<ASTNode> parseForStatement();
    Ptr<ASTNode> parseReturnStatement();
    
    Token consume(TokenType type, StringRef message);
    bool match(TokenType type);
    bool check(TokenType type) const;
    Token advance();
    bool isAtEnd() const;
    Token peek() const;
    Token previous() const;
    void sync();
};

} // namespace frontend
} // namespace ccvm

#endif
