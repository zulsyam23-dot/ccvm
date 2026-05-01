#include "ccvm/frontend/token.h"

namespace ccvm {
namespace frontend {

String Token::tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::INTEGER_LITERAL: return "INTEGER_LITERAL";
        case TokenType::FLOAT_LITERAL: return "FLOAT_LITERAL";
        case TokenType::STRING_LITERAL: return "STRING_LITERAL";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::PLUS: return "PLUS";
        case TokenType::MINUS: return "MINUS";
        case TokenType::MULTIPLY: return "MULTIPLY";
        case TokenType::DIVIDE: return "DIVIDE";
        case TokenType::ASSIGN: return "ASSIGN";
        case TokenType::EQUAL: return "EQUAL";
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::LPAREN: return "LPAREN";
        case TokenType::RPAREN: return "RPAREN";
        case TokenType::LBRACE: return "LBRACE";
        case TokenType::RBRACE: return "RBRACE";
        case TokenType::EOF_TOKEN: return "EOF";
        case TokenType::KW_INT: return "KW_INT";
        case TokenType::KW_FLOAT: return "KW_FLOAT";
        case TokenType::KW_DOUBLE: return "KW_DOUBLE";
        case TokenType::KW_CHAR: return "KW_CHAR";
        case TokenType::KW_VOID: return "KW_VOID";
        case TokenType::KW_IF: return "KW_IF";
        case TokenType::KW_ELSE: return "KW_ELSE";
        case TokenType::KW_WHILE: return "KW_WHILE";
        case TokenType::KW_FOR: return "KW_FOR";
        case TokenType::KW_RETURN: return "KW_RETURN";
        case TokenType::KW_STRUCT: return "KW_STRUCT";
        case TokenType::KW_LET: return "KW_LET";
        case TokenType::KW_MUT: return "KW_MUT";
        case TokenType::KW_FUNCTION: return "KW_FUNCTION";
        case TokenType::KW_MODULE: return "KW_MODULE";
        case TokenType::KW_END: return "KW_END";
        case TokenType::COMMA: return "COMMA";
        case TokenType::LESS: return "LESS";
        case TokenType::GREATER: return "GREATER";
        case TokenType::LESS_EQUAL: return "LESS_EQUAL";
        case TokenType::GREATER_EQUAL: return "GREATER_EQUAL";
        case TokenType::NOT_EQUAL: return "NOT_EQUAL";
        default: return "UNKNOWN";
    }
}

} // namespace frontend
} // namespace ccvm
