#include "ccvm/frontend/lexer.h"
#include "ccvm/frontend/token.h"
#include <map>

namespace ccvm::frontend {

Lexer::Lexer(Language language) 
    : current_language_(language) {
    config_ = LexerConfig::getDefault();
    config_.language = language;
}

Lexer::Lexer(const LexerConfig& config, Ptr<ErrorHandler> error_handler)
    : config_(config), error_handler_(error_handler), current_language_(config.language) {
}

Lexer::~Lexer() = default;

Vector<Token> Lexer::tokenize(StringRef source, StringRef filename) {
    state_ = std::make_unique<LexerState>(source.c_str(), source.length());
    state_->current_location.filename = filename;
    
    lex();
    
    return state_->tokens;
}

Vector<Token> Lexer::tokenizeFile(StringRef filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw LexerError("Could not open file: " + filename);
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return tokenize(buffer.str(), filename);
}

void Lexer::lex() {
    while (state_->current_offset < state_->source_length) {
        char c = state_->source[state_->current_offset];
        
        if (isWhitespace(c)) {
            skipWhitespace();
            continue;
        }
        
        if (c == '/' && peekChar() == '/') {
            extractComment();
            continue;
        }
        
        if (isDigit(c)) {
            state_->tokens.push_back(extractNumber());
            continue;
        }
        
        if (isAlpha(c)) {
            state_->tokens.push_back(extractIdentifier());
            continue;
        }
        
        if (c == '"') {
            state_->tokens.push_back(extractString());
            continue;
        }
        
        state_->tokens.push_back(extractOperator());
    }
    
    state_->tokens.push_back(createEOFToken());
}

char Lexer::currentChar() const {
    if (state_->current_offset >= state_->source_length) return '\0';
    return state_->source[state_->current_offset];
}

char Lexer::peekChar(size_t offset) const {
    if (state_->current_offset + offset >= state_->source_length) return '\0';
    return state_->source[state_->current_offset + offset];
}

char Lexer::advance() {
    char c = state_->source[state_->current_offset++];
    if (c == '\n') {
        state_->current_line++;
        state_->current_column = 1;
    } else {
        state_->current_column++;
    }
    state_->current_location.line = state_->current_line;
    state_->current_location.column = state_->current_column;
    state_->current_location.offset = state_->current_offset;
    return c;
}

void Lexer::advance(size_t count) {
    for (size_t i = 0; i < count; ++i) {
        advance();
    }
}

bool Lexer::isEOF() const {
    return state_->current_offset >= state_->source_length;
}

void Lexer::skipWhitespace() {
    while (state_->current_offset < state_->source_length && isWhitespace(state_->source[state_->current_offset])) {
        advance();
    }
}

Token Lexer::extractNumber() {
    SourceLocation start_loc = state_->current_location;
    String raw;
    bool is_float = false;
    
    while (state_->current_offset < state_->source_length && 
           (isDigit(state_->source[state_->current_offset]) || state_->source[state_->current_offset] == '.')) {
        char c = state_->source[state_->current_offset];
        if (c == '.') {
            if (is_float) break;
            is_float = true;
        }
        raw += advance();
    }
    
    if (is_float) {
        return Token(TokenType::FLOAT_LITERAL, std::stod(raw), start_loc, raw, current_language_);
    } else {
        return Token(TokenType::INTEGER_LITERAL, (IntType)std::stoll(raw), start_loc, raw, current_language_);
    }
}

Token Lexer::extractIdentifier() {
    static const std::map<String, TokenType> keywords = {
        {"int", TokenType::KW_INT},
        {"float", TokenType::KW_FLOAT},
        {"double", TokenType::KW_DOUBLE},
        {"char", TokenType::KW_CHAR},
        {"void", TokenType::KW_VOID},
        {"if", TokenType::KW_IF},
        {"else", TokenType::KW_ELSE},
        {"while", TokenType::KW_WHILE},
        {"for", TokenType::KW_FOR},
        {"return", TokenType::KW_RETURN},
        {"struct", TokenType::KW_STRUCT},
        {"let", TokenType::KW_LET},
        {"mut", TokenType::KW_MUT},
        {"fn", TokenType::KW_FUNCTION},
        {"module", TokenType::KW_MODULE},
        {"end", TokenType::KW_END}
    };

    SourceLocation start_loc = state_->current_location;
    String raw;
    
    while (state_->current_offset < state_->source_length && isAlphaNumeric(state_->source[state_->current_offset])) {
        raw += advance();
    }
    
    auto it = keywords.find(raw);
    if (it != keywords.end()) {
        return Token(it->second, raw, start_loc, raw, current_language_);
    }
    
    return Token(TokenType::IDENTIFIER, raw, start_loc, raw, current_language_);
}

Token Lexer::extractString() {
    SourceLocation start_loc = state_->current_location;
    advance();
    String raw;
    
    while (state_->current_offset < state_->source_length && state_->source[state_->current_offset] != '"') {
        raw += advance();
    }
    
    if (state_->current_offset < state_->source_length) {
        advance();
    }
    
    return Token(TokenType::STRING_LITERAL, raw, start_loc, raw, current_language_);
}

Token Lexer::extractOperator() {
    SourceLocation start_loc = state_->current_location;
    char c = advance();
    String raw(1, c);
    TokenType type = TokenType::UNKNOWN;
    
    switch (c) {
        case '+': 
            if (state_->current_offset < state_->source_length && state_->source[state_->current_offset] == '=') {
                advance(); raw += '='; type = TokenType::PLUS_ASSIGN;
            } else if (state_->current_offset < state_->source_length && state_->source[state_->current_offset] == '+') {
                advance(); raw += '+'; type = TokenType::INCREMENT;
            } else {
                type = TokenType::PLUS;
            }
            break;
        case '-':
            if (state_->current_offset < state_->source_length && state_->source[state_->current_offset] == '=') {
                advance(); raw += '='; type = TokenType::MINUS_ASSIGN;
            } else if (state_->current_offset < state_->source_length && state_->source[state_->current_offset] == '-') {
                advance(); raw += '-'; type = TokenType::DECREMENT;
            } else if (state_->current_offset < state_->source_length && state_->source[state_->current_offset] == '>') {
                advance(); raw += '>'; type = TokenType::ARROW;
            } else {
                type = TokenType::MINUS;
            }
            break;
        case '*': type = TokenType::MULTIPLY; break;
        case '/': type = TokenType::DIVIDE; break;
        case '=': 
            if (state_->current_offset < state_->source_length && state_->source[state_->current_offset] == '=') {
                advance(); raw += '='; type = TokenType::EQUAL;
            } else {
                type = TokenType::ASSIGN;
            }
            break;
        case ';': type = TokenType::SEMICOLON; break;
        case ',': type = TokenType::COMMA; break;
        case '<': 
            if (state_->current_offset < state_->source_length && state_->source[state_->current_offset] == '=') {
                advance(); raw += '='; type = TokenType::LESS_EQUAL;
            } else if (state_->current_offset < state_->source_length && state_->source[state_->current_offset] == '<') {
                advance(); raw += '<'; type = TokenType::LEFT_SHIFT;
            } else {
                type = TokenType::LESS;
            }
            break;
        case '>':
            if (state_->current_offset < state_->source_length && state_->source[state_->current_offset] == '=') {
                advance(); raw += '='; type = TokenType::GREATER_EQUAL;
            } else if (state_->current_offset < state_->source_length && state_->source[state_->current_offset] == '>') {
                advance(); raw += '>'; type = TokenType::RIGHT_SHIFT;
            } else {
                type = TokenType::GREATER;
            }
            break;
        case '(': type = TokenType::LPAREN; break;
        case ')': type = TokenType::RPAREN; break;
        case '{': type = TokenType::LBRACE; break;
        case '}': type = TokenType::RBRACE; break;
        case '[': type = TokenType::LBRACKET; break;
        case ']': type = TokenType::RBRACKET; break;
        case ':': 
            if (state_->current_offset < state_->source_length && state_->source[state_->current_offset] == ':') {
                advance(); raw += ':'; type = TokenType::DOUBLE_COLON;
            } else {
                type = TokenType::COLON;
            }
            break;
    }
    
    return Token(type, raw, start_loc, raw, current_language_);
}

Token Lexer::extractComment() {
    SourceLocation start_loc = state_->current_location;
    advance();
    advance();
    
    while (state_->current_offset < state_->source_length && state_->source[state_->current_offset] != '\n') {
        advance();
    }
    
    return createToken(TokenType::COMMENT);
}

Token Lexer::extractCLiteral() {
    return extractToken();
}

Token Lexer::extractCppLiteral() {
    return extractToken();
}

Token Lexer::extractRustLiteral() {
    return extractToken();
}

Token Lexer::extractJuliaLiteral() {
    return extractToken();
}

Token Lexer::extractToken() {
    return createToken(TokenType::UNKNOWN);
}

Token Lexer::createToken(TokenType type) {
    return Token(type, state_->current_location, current_language_);
}

Token Lexer::createToken(TokenType type, const TokenValue& value) {
    return Token(type, value, state_->current_location, "", current_language_);
}

Token Lexer::createToken(TokenType type, const TokenValue& value, const String& raw_text) {
    return Token(type, value, state_->current_location, raw_text, current_language_);
}

Token Lexer::createErrorToken(const String& message) {
    return Token(TokenType::ERROR_TOKEN, message, state_->current_location, message, current_language_);
}

Token Lexer::createEOFToken() {
    return Token(TokenType::EOF_TOKEN, state_->current_location, current_language_);
}

} // namespace ccvm::frontend
