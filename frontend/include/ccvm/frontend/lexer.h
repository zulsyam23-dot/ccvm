#ifndef CCVM_FRONTEND_LEXER_H
#define CCVM_FRONTEND_LEXER_H

#include "common.h"
#include "token.h"
#include <memory>

namespace ccvm {
namespace frontend {

class ErrorHandler;

struct LexerConfig {
    Language language = Language::C;
    bool enable_comments = true;
    bool enable_preprocessor = true;
    bool enable_unicode = true;
    bool enable_hex_floats = false;
    bool enable_binary_literals = false;
    bool track_locations = true;
    bool enable_warnings = true;
    
    static LexerConfig getDefault() {
        LexerConfig config;
        config.language = Language::C;
        config.enable_comments = true;
        config.enable_preprocessor = true;
        config.enable_unicode = true;
        config.enable_hex_floats = false;
        config.enable_binary_literals = false;
        config.track_locations = true;
        config.enable_warnings = true;
        return config;
    }
};

struct LexerState {
    const char* source;
    size_t source_length;
    size_t current_offset;
    size_t current_line;
    size_t current_column;
    SourceLocation current_location;
    Vector<Token> tokens;
    Vector<String> errors;
    Vector<String> warnings;
    
    LexerState(const char* src, size_t length) 
        : source(src), source_length(length), current_offset(0), 
          current_line(1), current_column(1) {}
};

class Lexer {
private:
    UniquePtr<LexerState> state_;
    LexerConfig config_;
    Ptr<ErrorHandler> error_handler_;
    Language current_language_;
    
public:
    explicit Lexer(Language language = Language::C);
    Lexer(const LexerConfig& config, Ptr<ErrorHandler> error_handler = nullptr);
    ~Lexer();
    
    Vector<Token> tokenize(StringRef source, StringRef filename = "");
    Vector<Token> tokenizeFile(StringRef filename);
    
    void setConfig(const LexerConfig& config) { config_ = config; }
    const LexerConfig& getConfig() const { return config_; }
    
    void setLanguage(Language language) { current_language_ = language; }
    Language getLanguage() const { return current_language_; }
    
    void setErrorHandler(Ptr<ErrorHandler> handler) { error_handler_ = handler; }
    const Vector<String>& getErrors() const { return state_->errors; }
    const Vector<String>& getWarnings() const { return state_->warnings; }
    bool hasErrors() const { return !state_->errors.empty(); }
    bool hasWarnings() const { return !state_->warnings.empty(); }
    
    size_t getTokenCount() const { return state_->tokens.size(); }
    size_t getSourceLength() const { return state_->source_length; }
    
private:
    void lex();
    
    bool isEOF() const;
    char currentChar() const;
    char peekChar(size_t offset = 1) const;
    char advance();
    void advance(size_t count);
    
    Token extractToken();
    Token extractNumber();
    Token extractString();
    Token extractChar();
    Token extractIdentifier();
    virtual Token extractOperator();
    virtual Token extractComment();
    Token extractPreprocessor();
    Token extractWhitespace();
    
    Token parseInteger();
    Token parseFloat();
    Token parseHexNumber();
    Token parseOctalNumber();
    Token parseBinaryNumber();
    
    Token parseString();
    Token parseRawString();
    Token parseChar();
    String parseEscapeSequence();
    
    Token processIdentifier(const String& identifier);
    Token processKeyword(const String& keyword);
    bool isKeyword(const String& word) const;
    TokenType getKeywordType(const String& word) const;
    
    virtual Token extractCLiteral();
    Token extractCppLiteral();
    Token extractRustLiteral();
    Token extractJuliaLiteral();
    
    TokenType getOperatorType(char first, char second = '\0', char third = '\0') const;
    Token extractMultiCharOperator();
    
    Token extractSingleLineComment();
    Token extractMultiLineComment();
    Token extractDocComment();
    
    Token extractPreprocessorDirective();
    Token extractPreprocessorArgument();
    
    void updateLocation();
    SourceLocation getCurrentLocation() const;
    SourceRange getCurrentRange() const;
    
    void reportError(const String& message);
    void reportWarning(const String& message);
    void reportError(const String& message, const SourceLocation& loc);
    void reportWarning(const String& message, const SourceLocation& loc);
    
    bool match(char expected);
    bool match(const String& expected);
    void skipWhitespace();
    void skipLine();
    String getCurrentLexeme() const;
    size_t getCurrentOffset() const { return state_->current_offset; }
    
    bool isCLetter(char c) const;
    bool isCppLetter(char c) const;
    bool isRustLetter(char c) const;
    bool isJuliaLetter(char c) const;
    bool isNumberStart(char c) const;
    bool isIdentifierStart(char c) const;
    bool isIdentifierPart(char c) const;
    
    bool isUnicodeIdentifierStart(uint32_t codepoint) const;
    bool isUnicodeIdentifierPart(uint32_t codepoint) const;
    uint32_t readUTF8Codepoint();
    
    Token createToken(TokenType type);
    Token createToken(TokenType type, const TokenValue& value);
    Token createToken(TokenType type, const TokenValue& value, const String& raw_text);
    Token createErrorToken(const String& message);
    Token createEOFToken();
    
    void initializeKeywords();
    Map<String, TokenType> c_keywords_;
    Map<String, TokenType> cpp_keywords_;
    Map<String, TokenType> rust_keywords_;
    Map<String, TokenType> julia_keywords_;
    Map<String, TokenType> type_keywords_;
};

class CLexer : public Lexer {
public:
    CLexer() : Lexer(Language::C) {}
};

class CppLexer : public Lexer {
public:
    CppLexer() : Lexer(Language::Cpp) {}
};

class RustLexer : public Lexer {
public:
    RustLexer() : Lexer(Language::Rust) {}
};

class JuliaLexer : public Lexer {
public:
    JuliaLexer() : Lexer(Language::Julia) {}
};

UniquePtr<Lexer> createLexer(Language language);

} // namespace frontend
} // namespace ccvm

#endif // CCVM_FRONTEND_LEXER_H
