#include "ccvm/frontend/parser.h"
#include <stdexcept>

namespace ccvm::frontend {

static PrimitiveType::Kind tokenToPrimitiveKind(TokenType type) {
    switch (type) {
        case TokenType::KW_INT: return PrimitiveType::Kind::INT;
        case TokenType::KW_FLOAT: return PrimitiveType::Kind::FLOAT;
        case TokenType::KW_VOID: return PrimitiveType::Kind::VOID;
        case TokenType::KW_CHAR: return PrimitiveType::Kind::CHAR;
        case TokenType::KW_DOUBLE: return PrimitiveType::Kind::DOUBLE;
        case TokenType::KW_BOOL: return PrimitiveType::Kind::BOOL;
        default: return PrimitiveType::Kind::INT;
    }
}

Parser::Parser(Lexer& lexer) {
    tokens_ = lexer.tokenize("", "");
    current_token_idx_ = 0;
}

Parser::Parser(Vector<Token> tokens) : tokens_(std::move(tokens)) {
    current_token_idx_ = 0;
}

Ptr<ASTNode> Parser::parse() {
    auto translation_unit = std::make_shared<TranslationUnit>();
    
    while (!isAtEnd()) {
        try {
            auto decl = parseDeclaration();
            if (decl) {
                translation_unit->addDeclaration(decl);
            }
        } catch (const ParserError& e) {
            sync();
        }
    }
    
    return translation_unit;
}

Ptr<ASTNode> Parser::parseDeclaration() {
    if (match(TokenType::KW_INT) || match(TokenType::KW_FLOAT) || match(TokenType::KW_VOID)) {
        Token type_token = previous();
        Token name_token = consume(TokenType::IDENTIFIER, "Expect identifier after type");
        
        if (match(TokenType::LPAREN)) {
            return parseFunctionDefinition(type_token, name_token);
        } else {
            return parseVariableDeclaration(type_token, name_token);
        }
    }
    
    throw ParserError("Expected declaration");
}

Ptr<ASTNode> Parser::parseFunctionDefinition(Token type, Token name) {
    auto func_type = std::make_shared<PrimitiveType>(tokenToPrimitiveKind(type.type()));
    Vector<Ptr<Declaration>> parameters;
    
    if (!check(TokenType::RPAREN)) {
        do {
            Token p_type = consume(TokenType::KW_INT, "Expect parameter type");
            Token p_name = consume(TokenType::IDENTIFIER, "Expect parameter name");
            auto param_type = std::make_shared<PrimitiveType>(tokenToPrimitiveKind(p_type.type()));
            auto param = std::make_shared<VariableDeclaration>(p_name.raw_text(), param_type);
            parameters.push_back(param);
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RPAREN, "Expect ')' after parameters");
    
    consume(TokenType::LBRACE, "Expect '{' before function body");
    auto body = std::dynamic_pointer_cast<Statement>(parseCompoundStatement());
    auto func = std::make_shared<FunctionDefinition>(name.raw_text(), func_type, body);
    
    for (const auto& param : parameters) {
        func->getParameters().push_back(param);
    }
    
    return func;
}

Ptr<ASTNode> Parser::parseVariableDeclaration(Token type, Token name) {
    auto var_type = std::make_shared<PrimitiveType>(tokenToPrimitiveKind(type.type()));
    auto var = std::make_shared<VariableDeclaration>(name.raw_text(), var_type);
    
    if (match(TokenType::ASSIGN)) {
        auto init = std::dynamic_pointer_cast<Expression>(parseExpression());
        var->setInitializer(init);
    }
    
    consume(TokenType::SEMICOLON, "Expect ';' after variable declaration");
    return var;
}

Ptr<ASTNode> Parser::parseCompoundStatement() {
    auto block = std::make_shared<CompoundStatement>();
    
    while (!check(TokenType::RBRACE) && !isAtEnd()) {
        if (match(TokenType::KW_INT) || match(TokenType::KW_FLOAT) || match(TokenType::KW_VOID)) {
            Token type_token = previous();
            Token name_token = consume(TokenType::IDENTIFIER, "Expect identifier after type");
            auto decl = parseVariableDeclaration(type_token, name_token);
            if (decl) {
                block->addChild(decl);
            }
        } else {
            auto stmt = parseStatement();
            block->addChild(stmt);
        }
    }
    
    consume(TokenType::RBRACE, "Expect '}' after block");
    return block;
}

Ptr<ASTNode> Parser::parseStatement() {
    if (match(TokenType::KW_IF)) return parseIfStatement();
    if (match(TokenType::KW_WHILE)) return parseWhileStatement();
    if (match(TokenType::KW_FOR)) return parseForStatement();
    if (match(TokenType::KW_RETURN)) return parseReturnStatement();
    if (match(TokenType::LBRACE)) return parseCompoundStatement();
    
    return parseExpressionStatement();
}

Ptr<ASTNode> Parser::parseExpressionStatement() {
    auto expr = parseExpression();
    consume(TokenType::SEMICOLON, "Expect ';' after expression");
    return std::make_shared<ExpressionStatement>(std::dynamic_pointer_cast<Expression>(expr));
}

Ptr<ASTNode> Parser::parseExpression() {
    return parseAssignment();
}

Ptr<ASTNode> Parser::parseAssignment() {
    auto expr = parseBinary();
    
    if (match(TokenType::ASSIGN)) {
        auto value = parseAssignment();
        return std::make_shared<BinaryExpression>(BinaryExpression::Operator::ASSIGN, 
                                                   std::dynamic_pointer_cast<Expression>(expr), 
                                                   std::dynamic_pointer_cast<Expression>(value));
    }
    
    return expr;
}

Ptr<ASTNode> Parser::parseBinary() {
    auto left = parseComparison();
    
    while (match(TokenType::PLUS) || match(TokenType::MINUS) || match(TokenType::MULTIPLY) || match(TokenType::DIVIDE)) {
        Token op = previous();
        auto right = parseComparison();
        
        BinaryExpression::Operator bin_op = BinaryExpression::Operator::ADD;
        if (op.type() == TokenType::PLUS) bin_op = BinaryExpression::Operator::ADD;
        else if (op.type() == TokenType::MINUS) bin_op = BinaryExpression::Operator::SUB;
        else if (op.type() == TokenType::MULTIPLY) bin_op = BinaryExpression::Operator::MUL;
        else if (op.type() == TokenType::DIVIDE) bin_op = BinaryExpression::Operator::DIV;
        
        left = std::make_shared<BinaryExpression>(bin_op, 
                                                   std::dynamic_pointer_cast<Expression>(left), 
                                                   std::dynamic_pointer_cast<Expression>(right));
    }
    
    return left;
}

Ptr<ASTNode> Parser::parseComparison() {
    auto left = parsePrimary();
    
    while (match(TokenType::LESS) || match(TokenType::GREATER) || 
           match(TokenType::LESS_EQUAL) || match(TokenType::GREATER_EQUAL) ||
           match(TokenType::EQUAL) || match(TokenType::NOT_EQUAL)) {
        Token op = previous();
        auto right = parsePrimary();
        
        BinaryExpression::Operator bin_op = BinaryExpression::Operator::LT;
        if (op.type() == TokenType::LESS) bin_op = BinaryExpression::Operator::LT;
        else if (op.type() == TokenType::GREATER) bin_op = BinaryExpression::Operator::GT;
        else if (op.type() == TokenType::LESS_EQUAL) bin_op = BinaryExpression::Operator::LE;
        else if (op.type() == TokenType::GREATER_EQUAL) bin_op = BinaryExpression::Operator::GE;
        else if (op.type() == TokenType::EQUAL) bin_op = BinaryExpression::Operator::EQ;
        else if (op.type() == TokenType::NOT_EQUAL) bin_op = BinaryExpression::Operator::NE;
        
        left = std::make_shared<BinaryExpression>(bin_op, 
                                                   std::dynamic_pointer_cast<Expression>(left), 
                                                   std::dynamic_pointer_cast<Expression>(right));
    }
    
    return left;
}

Ptr<ASTNode> Parser::parsePrimary() {
    if (match(TokenType::INTEGER_LITERAL)) {
        return std::make_shared<IntegerLiteral>(previous().get_integer());
    }
    if (match(TokenType::FLOAT_LITERAL)) {
        return std::make_shared<FloatLiteral>(previous().get_float());
    }
    if (match(TokenType::IDENTIFIER)) {
        Token ident = previous();
        if (match(TokenType::LPAREN)) {
            Vector<Ptr<Expression>> args;
            if (!check(TokenType::RPAREN)) {
                do {
                    auto arg = std::dynamic_pointer_cast<Expression>(parseExpression());
                    args.push_back(arg);
                } while (match(TokenType::COMMA));
            }
            consume(TokenType::RPAREN, "Expect ')' after function arguments");
            auto callee = std::make_shared<IdentifierExpression>(ident.raw_text());
            return std::make_shared<CallExpression>(callee, std::move(args));
        }
        return std::make_shared<IdentifierExpression>(ident.raw_text());
    }
    
    if (match(TokenType::LPAREN)) {
        auto expr = parseExpression();
        consume(TokenType::RPAREN, "Expect ')' after expression");
        return expr;
    }
    
    if (match(TokenType::STRING_LITERAL)) {
        return std::make_shared<StringLiteral>(previous().raw_text());
    }
    
    throw ParserError("Expected expression");
}

Ptr<ASTNode> Parser::parseIfStatement() {
    consume(TokenType::LPAREN, "Expect '(' after 'if'");
    auto cond = parseExpression();
    consume(TokenType::RPAREN, "Expect ')' after condition");
    auto then_stmt = parseStatement();
    
    Ptr<Statement> else_stmt = nullptr;
    if (match(TokenType::KW_ELSE)) {
        else_stmt = std::dynamic_pointer_cast<Statement>(parseStatement());
    }
    
    return std::make_shared<IfStatement>(std::dynamic_pointer_cast<Expression>(cond), 
                                          std::dynamic_pointer_cast<Statement>(then_stmt), 
                                          else_stmt);
}

Ptr<ASTNode> Parser::parseWhileStatement() {
    consume(TokenType::LPAREN, "Expect '(' after 'while'");
    auto cond = parseExpression();
    consume(TokenType::RPAREN, "Expect ')' after condition");
    auto body = parseStatement();
    
    return std::make_shared<WhileStatement>(std::dynamic_pointer_cast<Expression>(cond), 
                                             std::dynamic_pointer_cast<Statement>(body));
}

Ptr<ASTNode> Parser::parseForStatement() {
    consume(TokenType::LPAREN, "Expect '(' after 'for'");
    
    Ptr<Declaration> init = nullptr;
    if (match(TokenType::KW_INT) || match(TokenType::KW_FLOAT) || match(TokenType::KW_VOID)) {
        Token type_token = previous();
        Token name_token = consume(TokenType::IDENTIFIER, "Expect identifier after type");
        init = std::dynamic_pointer_cast<Declaration>(parseVariableDeclaration(type_token, name_token));
    } else if (!check(TokenType::SEMICOLON)) {
        auto expr = parseExpression();
        consume(TokenType::SEMICOLON, "Expect ';' after for initializer");
        init = std::dynamic_pointer_cast<Declaration>(expr);
    } else {
        advance();
    }
    
    Ptr<Expression> cond = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        cond = std::dynamic_pointer_cast<Expression>(parseExpression());
    }
    consume(TokenType::SEMICOLON, "Expect ';' after for condition");
    
    Ptr<Expression> inc = nullptr;
    if (!check(TokenType::RPAREN)) {
        inc = std::dynamic_pointer_cast<Expression>(parseExpression());
    }
    consume(TokenType::RPAREN, "Expect ')' after for increment");
    
    auto body = parseStatement();
    
    return std::make_shared<ForStatement>(init, cond, inc, std::dynamic_pointer_cast<Statement>(body));
}

Ptr<ASTNode> Parser::parseReturnStatement() {
    Ptr<Expression> value = nullptr;
    if (!check(TokenType::SEMICOLON)) {
        value = std::dynamic_pointer_cast<Expression>(parseExpression());
    }
    consume(TokenType::SEMICOLON, "Expect ';' after return");
    return std::make_shared<ReturnStatement>(value);
}

Token Parser::consume(TokenType type, StringRef message) {
    if (check(type)) return advance();
    throw ParserError(message);
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type() == type;
}

Token Parser::advance() {
    if (!isAtEnd()) current_token_idx_++;
    return previous();
}

bool Parser::isAtEnd() const {
    return peek().type() == TokenType::EOF_TOKEN;
}

Token Parser::peek() const {
    return tokens_[current_token_idx_];
}

Token Parser::previous() const {
    return tokens_[current_token_idx_ - 1];
}

void Parser::sync() {
    advance();
    while (!isAtEnd()) {
        if (previous().type() == TokenType::SEMICOLON) return;
        
        switch (peek().type()) {
            case TokenType::KW_INT:
            case TokenType::KW_FLOAT:
            case TokenType::KW_FUNCTION:
            case TokenType::KW_IF:
            case TokenType::KW_WHILE:
            case TokenType::KW_RETURN:
                return;
            default:
                break;
        }
        advance();
    }
}

} // namespace ccvm::frontend
