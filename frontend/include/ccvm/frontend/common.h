#ifndef CCVM_FRONTEND_COMMON_H
#define CCVM_FRONTEND_COMMON_H

#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cstdint>
#include <cassert>

namespace ccvm {
namespace frontend {

// Forward declarations
class Token;
class Lexer;
class Parser;
class ASTNode;
class ErrorHandler;

// Type aliases
using String = std::string;
using StringRef = const std::string&;
using StringPtr = std::shared_ptr<std::string>;
using IntType = int64_t;
using FloatType = double;
using BoolType = bool;

template<typename T>
using Ptr = std::shared_ptr<T>;

template<typename T>
using UniquePtr = std::unique_ptr<T>;

template<typename T>
using Vector = std::vector<T>;

template<typename K, typename V>
using Map = std::unordered_map<K, V>;

template<typename T>
using Set = std::unordered_set<T>;

// Utility functions
inline bool isWhitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

inline bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

inline bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

inline bool isAlphaNumeric(char c) {
    return isAlpha(c) || isDigit(c);
}

inline bool isHexDigit(char c) {
    return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

inline bool isOctalDigit(char c) {
    return c >= '0' && c <= '7';
}

inline bool isBinaryDigit(char c) {
    return c == '0' || c == '1';
}

// String utilities
inline String trim(const String& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == String::npos) return "";
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

inline Vector<String> split(const String& str, char delimiter) {
    Vector<String> result;
    std::stringstream ss(str);
    String item;
    
    while (std::getline(ss, item, delimiter)) {
        result.push_back(item);
    }
    
    return result;
}

inline String join(const Vector<String>& parts, const String& separator) {
    if (parts.empty()) return "";
    
    String result = parts[0];
    for (size_t i = 1; i < parts.size(); ++i) {
        result += separator + parts[i];
    }
    
    return result;
}

// Error handling
class CCVMError : public std::runtime_error {
public:
    explicit CCVMError(const String& message) 
        : std::runtime_error(message) {}
    
    explicit CCVMError(const char* message) 
        : std::runtime_error(message) {}
};

class LexerError : public CCVMError {
public:
    explicit LexerError(const String& message) 
        : CCVMError("Lexer error: " + message) {}
};

class ParserError : public CCVMError {
public:
    explicit ParserError(const String& message) 
        : CCVMError("Parser error: " + message) {}
};

class SemanticError : public CCVMError {
public:
    explicit SemanticError(const String& message) 
        : CCVMError("Semantic error: " + message) {}
};

// Source location information
struct SourceLocation {
    String filename;
    size_t line;
    size_t column;
    size_t offset;
    
    SourceLocation() : line(1), column(1), offset(0) {}
    
    SourceLocation(StringRef filename, size_t line, size_t column, size_t offset)
        : filename(filename), line(line), column(column), offset(offset) {}
    
    String toString() const {
        return filename + ":" + std::to_string(line) + ":" + std::to_string(column);
    }
};

// Source range information
struct SourceRange {
    SourceLocation start;
    SourceLocation end;
    
    SourceRange() = default;
    SourceRange(const SourceLocation& start, const SourceLocation& end)
        : start(start), end(end) {}
    
    String toString() const {
        return start.toString() + "-" + end.toString();
    }
};

// Language support
enum class Language {
    C,
    Cpp,
    Rust,
    Julia,
    Unknown
};

inline Language detectLanguage(StringRef filename) {
    if (filename.ends_with(".c")) return Language::C;
    if (filename.ends_with(".cpp") || filename.ends_with(".cxx") || 
        filename.ends_with(".cc") || filename.ends_with(".hpp")) return Language::Cpp;
    if (filename.ends_with(".rs")) return Language::Rust;
    if (filename.ends_with(".jl")) return Language::Julia;
    return Language::Unknown;
}

// Target architecture support
enum class TargetArch {
    X86_64,
    ARM64,
    RISCV32,
    RISCV64,
    Unknown
};

inline TargetArch detectTargetArch(StringRef arch) {
    if (arch == "x86_64" || arch == "amd64") return TargetArch::X86_64;
    if (arch == "aarch64" || arch == "arm64") return TargetArch::ARM64;
    if (arch == "riscv32") return TargetArch::RISCV32;
    if (arch == "riscv64") return TargetArch::RISCV64;
    return TargetArch::Unknown;
}

// Configuration options
struct FrontendConfig {
    Language language = Language::Unknown;
    TargetArch target_arch = TargetArch::X86_64;
    bool enable_optimizations = true;
    bool enable_debug_info = true;
    int optimization_level = 2; // O2
    Vector<String> include_paths;
    Vector<String> defines;
    Vector<String> warnings;
    
    static FrontendConfig getDefault() {
        FrontendConfig config;
        config.language = Language::C;
        config.target_arch = TargetArch::X86_64;
        config.enable_optimizations = true;
        config.enable_debug_info = true;
        config.optimization_level = 2;
        return config;
    }
};

} // namespace frontend
} // namespace ccvm

#endif // CCVM_FRONTEND_COMMON_H