#ifndef CCVM_FRONTEND_TOKEN_H
#define CCVM_FRONTEND_TOKEN_H

#include "common.h"
#include <variant>

namespace ccvm {
namespace frontend {

enum class TokenType {
    // Literals
    INTEGER_LITERAL,
    FLOAT_LITERAL,
    STRING_LITERAL,
    CHAR_LITERAL,
    BOOLEAN_LITERAL,
    NULL_LITERAL,
    
    // Identifiers and keywords
    IDENTIFIER,
    KEYWORD,
    TYPE_NAME,
    
    // Operators
    PLUS,
    MINUS,
    MULTIPLY,
    DIVIDE,
    MODULO,
    ASSIGN,
    PLUS_ASSIGN,
    MINUS_ASSIGN,
    MULT_ASSIGN,
    DIV_ASSIGN,
    MOD_ASSIGN,
    
    // Comparison operators
    EQUAL,
    NOT_EQUAL,
    LESS,
    GREATER,
    LESS_EQUAL,
    GREATER_EQUAL,
    
    // Logical operators
    LOGICAL_AND,
    LOGICAL_OR,
    LOGICAL_NOT,
    
    // Bitwise operators
    BITWISE_AND,
    BITWISE_OR,
    BITWISE_XOR,
    BITWISE_NOT,
    LEFT_SHIFT,
    RIGHT_SHIFT,
    
    // Punctuation
    SEMICOLON,
    COMMA,
    DOT,
    ARROW,
    COLON,
    DOUBLE_COLON,
    QUESTION,
    
    // Brackets
    LPAREN,
    RPAREN,
    LBRACE,
    RBRACE,
    LBRACKET,
    RBRACKET,
    
    // Special
    EOF_TOKEN,
    NEWLINE,
    WHITESPACE,
    COMMENT,
    PREPROCESSOR,
    
    // C/C++ specific
    HASH,
    BACKSLASH,
    SINGLE_QUOTE,
    DOUBLE_QUOTE,
    
    // Rust specific
    AT,
    DOLLAR,
    
    // Julia specific
    BACKTICK,
    
    // Multi-character operators
    ELLIPSIS,
    RANGE,
    INCREMENT,
    DECREMENT,
    
    // C/C++ keywords
    KW_AUTO,
    KW_BREAK,
    KW_CASE,
    KW_CHAR,
    KW_CONST,
    KW_CONTINUE,
    KW_DEFAULT,
    KW_DO,
    KW_DOUBLE,
    KW_ELSE,
    KW_ENUM,
    KW_EXTERN,
    KW_FLOAT,
    KW_FOR,
    KW_GOTO,
    KW_IF,
    KW_INT,
    KW_LONG,
    KW_REGISTER,
    KW_RETURN,
    KW_SHORT,
    KW_SIGNED,
    KW_SIZEOF,
    KW_STATIC,
    KW_STRUCT,
    KW_SWITCH,
    KW_TYPEDEF,
    KW_UNION,
    KW_UNSIGNED,
    KW_VOID,
    KW_VOLATILE,
    KW_WHILE,
    
    // C++ keywords
    KW_ASM,
    KW_CATCH,
    KW_CLASS,
    KW_DELETE,
    KW_FRIEND,
    KW_INLINE,
    KW_NEW,
    KW_OPERATOR,
    KW_PRIVATE,
    KW_PROTECTED,
    KW_PUBLIC,
    KW_TEMPLATE,
    KW_THIS,
    KW_THROW,
    KW_TRY,
    KW_VIRTUAL,
    
    // Rust keywords
    KW_AS,
    KW_CRATE,
    KW_DYN,
    KW_IMPL,
    KW_LET,
    KW_LOOP,
    KW_MATCH,
    KW_MOD,
    KW_MOVE,
    KW_MUT,
    KW_PUB,
    KW_REF,
    KW_SELF_TYPE,
    KW_SUPER,
    KW_TRAIT,
    KW_TYPE,
    KW_USE,
    KW_WHERE,
    
    // Julia keywords
    KW_ABSTRACT,
    KW_MODULE,
    KW_FUNCTION,
    KW_MACRO,
    KW_END,
    KW_BEGIN,
    KW_QUOTE,
    KW_ELSEIF,
    KW_FINALLY,
    KW_USING,
    KW_IMPORT,
    KW_EXPORT,
    KW_GLOBAL,
    KW_LOCAL,
    KW_TRUE,
    KW_FALSE,
    KW_BOOL,
    KW_STRING,
    KW_ARRAY,
    KW_VECTOR,
    KW_MAP,
    KW_SET,
    KW_INTERNAL,
    KW_THREAD_LOCAL,
    KW_NOINLINE,
    KW_ALWAYS_INLINE,
    KW_NORETURN,
    KW_CONSTEXPR,
    KW_MUTABLE,
    KW_RESTRICT,
    KW_ATOMIC,
    
    // Error token
    ERROR_TOKEN,
    
    // Unknown token
    UNKNOWN
};

using TokenValue = std::variant<
    IntType,
    FloatType,
    String,
    BoolType,
    char,
    std::nullptr_t
>;

class Token {
private:
    TokenType type_;
    TokenValue value_;
    SourceLocation location_;
    SourceRange range_;
    String raw_text_;
    Language language_;
    
    static String tokenTypeToString(TokenType type);
    
public:
    Token() : type_(TokenType::UNKNOWN), value_(std::nullptr_t{}), language_(Language::Unknown) {}
    
    Token(TokenType type, const SourceLocation& loc, Language lang = Language::Unknown)
        : type_(type), location_(loc), range_(loc, loc), language_(lang) {}
    
    Token(TokenType type, const TokenValue& value, const SourceLocation& loc, 
          const String& raw_text, Language lang = Language::Unknown)
        : type_(type), value_(value), location_(loc), range_(loc, loc), 
          raw_text_(raw_text), language_(lang) {}
    
    TokenType type() const { return type_; }
    const TokenValue& value() const { return value_; }
    const SourceLocation& location() const { return location_; }
    const SourceRange& range() const { return range_; }
    const String& raw_text() const { return raw_text_; }
    Language language() const { return language_; }
    
    void set_type(TokenType type) { type_ = type; }
    void set_value(const TokenValue& value) { value_ = value; }
    void set_location(const SourceLocation& loc) { location_ = loc; range_ = SourceRange(loc, loc); }
    void set_range(const SourceRange& range) { range_ = range; }
    void set_raw_text(const String& text) { raw_text_ = text; }
    void set_language(Language lang) { language_ = lang; }
    
    bool is_integer() const { return std::holds_alternative<IntType>(value_); }
    bool is_float() const { return std::holds_alternative<FloatType>(value_); }
    bool is_string() const { return std::holds_alternative<String>(value_); }
    bool is_boolean() const { return std::holds_alternative<BoolType>(value_); }
    bool is_character() const { return std::holds_alternative<char>(value_); }
    bool is_null() const { return std::holds_alternative<std::nullptr_t>(value_); }
    
    IntType get_integer() const {
        if (!is_integer()) throw std::bad_variant_access{};
        return std::get<IntType>(value_);
    }
    
    FloatType get_float() const {
        if (!is_float()) throw std::bad_variant_access{};
        return std::get<FloatType>(value_);
    }
    
    const String& get_string() const {
        if (!is_string()) throw std::bad_variant_access{};
        return std::get<String>(value_);
    }
    
    BoolType get_boolean() const {
        if (!is_boolean()) throw std::bad_variant_access{};
        return std::get<BoolType>(value_);
    }
    
    char get_character() const {
        if (!is_character()) throw std::bad_variant_access{};
        return std::get<char>(value_);
    }
    
    bool is_literal() const {
        return type_ == TokenType::INTEGER_LITERAL ||
               type_ == TokenType::FLOAT_LITERAL ||
               type_ == TokenType::STRING_LITERAL ||
               type_ == TokenType::CHAR_LITERAL ||
               type_ == TokenType::BOOLEAN_LITERAL ||
               type_ == TokenType::NULL_LITERAL;
    }
    
    bool is_operator() const {
        return type_ >= TokenType::PLUS && type_ <= TokenType::BITWISE_NOT;
    }
    
    bool is_comparison() const {
        return type_ >= TokenType::EQUAL && type_ <= TokenType::GREATER_EQUAL;
    }
    
    bool is_keyword() const {
        return type_ >= TokenType::KW_AUTO && type_ <= TokenType::KW_ATOMIC;
    }
    
    bool is_punctuation() const {
        return type_ >= TokenType::SEMICOLON && type_ <= TokenType::QUESTION;
    }
    
    bool is_bracket() const {
        return type_ >= TokenType::LPAREN && type_ <= TokenType::RBRACKET;
    }
    
    bool is_identifier() const {
        return type_ == TokenType::IDENTIFIER;
    }
    
    String toString() const {
        String result = "Token(";
        result += tokenTypeToString(type_);
        
        if (is_integer()) {
            result += ", " + std::to_string(get_integer());
        } else if (is_float()) {
            result += ", " + std::to_string(get_float());
        } else if (is_string()) {
            result += ", \"" + get_string() + "\"";
        } else if (is_boolean()) {
            result += ", " + String(get_boolean() ? "true" : "false");
        } else if (is_character()) {
            result += ", '" + String(1, get_character()) + "'";
        }
        
        result += ")";
        return result;
    }
    
    int getPrecedence() const {
        switch (type_) {
            case TokenType::MULTIPLY:
            case TokenType::DIVIDE:
            case TokenType::MODULO:
                return 12;
            case TokenType::PLUS:
            case TokenType::MINUS:
                return 11;
            case TokenType::LEFT_SHIFT:
            case TokenType::RIGHT_SHIFT:
                return 10;
            case TokenType::LESS:
            case TokenType::GREATER:
            case TokenType::LESS_EQUAL:
            case TokenType::GREATER_EQUAL:
                return 9;
            case TokenType::EQUAL:
            case TokenType::NOT_EQUAL:
                return 8;
            case TokenType::BITWISE_AND:
                return 7;
            case TokenType::BITWISE_XOR:
                return 6;
            case TokenType::BITWISE_OR:
                return 5;
            case TokenType::LOGICAL_AND:
                return 4;
            case TokenType::LOGICAL_OR:
                return 3;
            case TokenType::ASSIGN:
            case TokenType::PLUS_ASSIGN:
            case TokenType::MINUS_ASSIGN:
            case TokenType::MULT_ASSIGN:
            case TokenType::DIV_ASSIGN:
            case TokenType::MOD_ASSIGN:
                return 2;
            default:
                return 0;
        }
    }
    
    enum class Associativity {
        LEFT,
        RIGHT,
        NONE
    };
    
    Associativity getAssociativity() const {
        switch (type_) {
            case TokenType::ASSIGN:
            case TokenType::PLUS_ASSIGN:
            case TokenType::MINUS_ASSIGN:
            case TokenType::MULT_ASSIGN:
            case TokenType::DIV_ASSIGN:
            case TokenType::MOD_ASSIGN:
                return Associativity::RIGHT;
            case TokenType::PLUS:
            case TokenType::MINUS:
            case TokenType::MULTIPLY:
            case TokenType::DIVIDE:
            case TokenType::MODULO:
            case TokenType::LEFT_SHIFT:
            case TokenType::RIGHT_SHIFT:
            case TokenType::LESS:
            case TokenType::GREATER:
            case TokenType::LESS_EQUAL:
            case TokenType::GREATER_EQUAL:
            case TokenType::EQUAL:
            case TokenType::NOT_EQUAL:
            case TokenType::BITWISE_AND:
            case TokenType::BITWISE_OR:
            case TokenType::BITWISE_XOR:
            case TokenType::LOGICAL_AND:
            case TokenType::LOGICAL_OR:
                return Associativity::LEFT;
            default:
                return Associativity::NONE;
        }
    }
};

class TokenStream {
private:
    Vector<Token> tokens_;
    size_t current_;
    
public:
    explicit TokenStream(const Vector<Token>& tokens) 
        : tokens_(tokens), current_(0) {}
    
    bool hasNext() const { return current_ < tokens_.size(); }
    bool hasPrevious() const { return current_ > 0; }
    
    const Token& current() const {
        if (current_ >= tokens_.size()) {
            static Token eof_token(TokenType::EOF_TOKEN, SourceLocation(), Language::Unknown);
            return eof_token;
        }
        return tokens_[current_];
    }
    
    const Token& peek(size_t offset = 1) const {
        size_t index = current_ + offset;
        if (index >= tokens_.size()) {
            static Token eof_token(TokenType::EOF_TOKEN, SourceLocation(), Language::Unknown);
            return eof_token;
        }
        return tokens_[index];
    }
    
    const Token& previous() const {
        if (current_ == 0) {
            static Token eof_token(TokenType::EOF_TOKEN, SourceLocation(), Language::Unknown);
            return eof_token;
        }
        return tokens_[current_ - 1];
    }
    
    const Token& next() {
        if (hasNext()) {
            ++current_;
        }
        return current();
    }
    
    const Token& advance() {
        return next();
    }
    
    void consume(TokenType expected_type) {
        if (current().type() != expected_type) {
            throw ParserError("Expected token type mismatch");
        }
        next();
    }
    
    bool match(TokenType type) {
        if (current().type() == type) {
            next();
            return true;
        }
        return false;
    }
    
    bool matchAny(const Vector<TokenType>& types) {
        for (auto type : types) {
            if (match(type)) return true;
        }
        return false;
    }
    
    bool check(TokenType type) const {
        return current().type() == type;
    }
    
    bool checkAny(const Vector<TokenType>& types) const {
        return std::find(types.begin(), types.end(), current().type()) != types.end();
    }
    
    size_t position() const { return current_; }
    void setPosition(size_t pos) { current_ = pos; }
    void reset() { current_ = 0; }
    
    bool isAtEnd() const { return current_ >= tokens_.size() || current().type() == TokenType::EOF_TOKEN; }
    size_t remaining() const { return tokens_.size() - current_; }
    
    void synchronize(const Vector<TokenType>& synchronizing_tokens) {
        while (!isAtEnd() && !checkAny(synchronizing_tokens)) {
            next();
        }
    }
};

} // namespace frontend
} // namespace ccvm

#endif // CCVM_FRONTEND_TOKEN_H
