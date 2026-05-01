#include "ccvm/frontend/common.h"
#include "ccvm/frontend/lexer.h"
#include "ccvm/frontend/parser.h"
#include "ccvm/frontend/semantic_analyzer.h"
#include "ccvm/frontend/ir_generator.h"
#include "ccvm/backend/codegen.h"
#include "ccvm/backend/ir_parser.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

namespace ccvm {
namespace frontend {

enum class OutputMode {
    Tokens,
    AST,
    IR,
    Assembly,
    Object,
    Executable
};

struct CompilerOptions {
    String input_file;
    String output_file;
    OutputMode output_mode = OutputMode::Executable;
    TargetArch target = TargetArch::X86_64;
    int opt_level = 0;
    bool verbose = false;
    bool print_timing = false;
};

static String read_file(const String& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

static void write_file(const String& path, const String& content) {
    std::ofstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot write to file: " + path);
    }
    file << content;
}

static CompilerOptions parse_args(int argc, char** argv) {
    CompilerOptions opts;
    
    for (int i = 1; i < argc; ++i) {
        String arg = argv[i];
        
        if (arg == "-o" && i + 1 < argc) {
            opts.output_file = argv[++i];
        } else if (arg == "--tokens") {
            opts.output_mode = OutputMode::Tokens;
        } else if (arg == "--ast") {
            opts.output_mode = OutputMode::AST;
        } else if (arg == "--ir") {
            opts.output_mode = OutputMode::IR;
        } else if (arg == "--asm") {
            opts.output_mode = OutputMode::Assembly;
        } else if (arg == "-O0") {
            opts.opt_level = 0;
        } else if (arg == "-O1") {
            opts.opt_level = 1;
        } else if (arg == "-O2") {
            opts.opt_level = 2;
        } else if (arg == "--verbose") {
            opts.verbose = true;
        } else if (arg == "--timing") {
            opts.print_timing = true;
        } else if (arg[0] != '-') {
            opts.input_file = arg;
        }
    }
    
    if (opts.input_file.empty()) {
        throw std::runtime_error("No input file specified");
    }
    
    if (opts.output_file.empty()) {
        if (opts.output_mode == OutputMode::Executable) {
            opts.output_file = "output.exe";
        } else if (opts.output_mode == OutputMode::Assembly) {
            opts.output_file = "output.asm";
        } else if (opts.output_mode == OutputMode::IR) {
            opts.output_file = "output.ir";
        } else {
            opts.output_file = "output.txt";
        }
    }
    
    return opts;
}

static void print_tokens(const Vector<Token>& tokens) {
    std::cout << "=== Tokens ===" << std::endl;
    for (const auto& token : tokens) {
        std::cout << "  " << token.toString() << std::endl;
    }
    std::cout << "  Total: " << tokens.size() << " tokens" << std::endl;
}

static void print_ast(const ASTNode& node, int indent = 0) {
    String prefix(indent * 2, ' ');
    std::cout << prefix << node.toString() << std::endl;
    
    auto tu = dynamic_cast<const ccvm::frontend::TranslationUnit*>(&node);
    if (tu) {
        for (const auto& child : tu->getDeclarations()) {
            if (child) {
                print_ast(*child, indent + 1);
            }
        }
    }
    
    auto func_def = dynamic_cast<const ccvm::frontend::FunctionDefinition*>(&node);
    if (func_def && func_def->getBody()) {
        print_ast(*func_def->getBody(), indent + 1);
    }
    
    auto compound = dynamic_cast<const ccvm::frontend::CompoundStatement*>(&node);
    if (compound) {
        for (const auto& stmt : compound->getStatements()) {
            if (stmt) {
                print_ast(*stmt, indent + 1);
            }
        }
    }
    
    auto var_decl = dynamic_cast<const ccvm::frontend::VariableDeclaration*>(&node);
    if (var_decl && var_decl->getInitializer()) {
        print_ast(*var_decl->getInitializer(), indent + 1);
    }
    
    auto binary = dynamic_cast<const ccvm::frontend::BinaryExpression*>(&node);
    if (binary) {
        print_ast(*binary->getLeft(), indent + 1);
        print_ast(*binary->getRight(), indent + 1);
    }
    
    for (const auto& child : node.getChildren()) {
        if (child) {
            print_ast(*child, indent + 1);
        }
    }
}

} // namespace frontend
} // namespace ccvm

int main(int argc, char** argv) {
    using namespace ccvm::frontend;
    
    std::cout << "CCVM Compiler v0.1.0" << std::endl;
    
    CompilerOptions opts;
    try {
        opts = parse_args(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << "Usage: ccvm-frontend <input_file> [options]" << std::endl;
        std::cerr << "Options:" << std::endl;
        std::cerr << "  -o <file>    Output file" << std::endl;
        std::cerr << "  --tokens     Print tokens" << std::endl;
        std::cerr << "  --ast        Print AST" << std::endl;
        std::cerr << "  --ir         Output IR" << std::endl;
        std::cerr << "  --asm        Output assembly" << std::endl;
        std::cerr << "  -O0/-O1/-O2  Optimization level" << std::endl;
        std::cerr << "  --verbose    Verbose output" << std::endl;
        return 1;
    }
    
    try {
        if (opts.verbose) {
            std::cout << "Input: " << opts.input_file << std::endl;
            std::cout << "Output: " << opts.output_file << std::endl;
            std::cout << "Target: x86_64" << std::endl;
        }
        
        Language lang = detectLanguage(opts.input_file);
        if (lang == Language::Unknown) {
            lang = Language::C;
        }
        
        String source = read_file(opts.input_file);
        
        if (opts.verbose) {
            std::cout << "Language: C/C++" << std::endl;
            std::cout << "Source size: " << source.size() << " bytes" << std::endl;
        }
        
        Lexer lexer(lang);
        Vector<Token> tokens = lexer.tokenize(source, opts.input_file);
        
        if (opts.output_mode == OutputMode::Tokens) {
            print_tokens(tokens);
            return 0;
        }
        
        if (lexer.hasErrors()) {
            std::cerr << "Lexer errors:" << std::endl;
            for (const auto& error : lexer.getErrors()) {
                std::cerr << "  " << error << std::endl;
            }
            return 1;
        }
        
        if (opts.verbose) {
            std::cout << "Lexed " << tokens.size() << " tokens" << std::endl;
        }
        
        Parser parser(tokens);
        Ptr<ASTNode> ast = parser.parse();
        
        if (opts.output_mode == OutputMode::AST) {
            std::cout << "=== AST ===" << std::endl;
            if (ast) {
                print_ast(*ast);
            } else {
                std::cout << "  (empty)" << std::endl;
            }
            return 0;
        }
        
        if (opts.verbose) {
            std::cout << "Parsed AST successfully" << std::endl;
        }
        
        SemanticAnalyzer analyzer;
        if (ast) {
            analyzer.analyze(*ast);
        }
        
        IRGenerator ir_gen;
        if (ast) {
            ir_gen.generate(*ast);
        }
        
        String ir_output = ir_gen.get_ir();
        
        if (opts.output_mode == OutputMode::IR) {
            std::cout << ir_output;
        }
        
        write_file(opts.output_file, ir_output);

        /* Backend: generate assembly from IR */
        if (opts.output_mode == OutputMode::Assembly || opts.output_mode == OutputMode::Object || opts.output_mode == OutputMode::Executable) {
            String asm_file = opts.output_file;
            if (opts.output_mode == OutputMode::Assembly) {
                asm_file = opts.output_file;
            } else {
                asm_file = opts.output_file.substr(0, opts.output_file.find_last_of('.')) + ".asm";
            }

            ccvm_ir_module_t* ir_module = new ccvm_ir_module_t();
            if (!ccvm_ir_parse(opts.output_file.c_str(), ir_module)) {
                std::cerr << "Error: Failed to parse IR" << std::endl;
                delete ir_module;
                return 1;
            }

            ccvm_backend_config config;
            config.arch = CCVM_TARGET_X86_64;
            config.optimization_level = opts.opt_level;
            config.debug_info = false;

            if (!ccvm_backend_generate(ir_module, &config, asm_file.c_str())) {
                std::cerr << "Error: Failed to generate assembly" << std::endl;
                delete ir_module;
                return 1;
            }
            delete ir_module;

            if (opts.verbose) {
                std::cout << "Generated assembly: " << asm_file << std::endl;
            }

            if (opts.output_mode == OutputMode::Assembly) {
                std::cout << "Assembly successful: " << asm_file << std::endl;
                return 0;
            }

            /* Assemble with ML64 */
            String obj_file = opts.output_file.substr(0, opts.output_file.find_last_of('.')) + ".obj";
            String ml64_path = "C:\\PROGRA~2\\MICROS~3\\2022\\BUILDT~1\\VC\\Tools\\MSVC\\1444~1.352\\bin\\Hostx64\\x64\\ml64.exe";
            String asm_cmd = "\"" + ml64_path + "\" /c /Fo " + obj_file + " " + asm_file;
            if (opts.verbose) {
                std::cout << "Running: " << asm_cmd << std::endl;
            }
            int asm_result = system(asm_cmd.c_str());
            if (asm_result != 0) {
                std::cerr << "Error: Assembly failed (ML64)" << std::endl;
                return 1;
            }

            if (opts.verbose) {
                std::cout << "Generated object: " << obj_file << std::endl;
            }

            if (opts.output_mode == OutputMode::Object) {
                std::cout << "Object compilation successful: " << obj_file << std::endl;
                return 0;
            }

            /* Link to produce executable */
            String exe_file = opts.output_file;
            if (exe_file.size() < 4 || exe_file.substr(exe_file.size() - 4) != ".exe") {
                exe_file += ".exe";
            }
            String kernel32_lib = "C:\\PROGRA~2\\WI3CF2~1\\10\\Lib\\100261~1.0\\um\\x64\\kernel32.lib";
            String link_path = "C:\\PROGRA~2\\MICROS~3\\2022\\BUILDT~1\\VC\\Tools\\MSVC\\1444~1.352\\bin\\Hostx64\\x64\\link.exe";
            String link_cmd = link_path + " /OUT:" + exe_file + " /SUBSYSTEM:CONSOLE " + obj_file + " " + kernel32_lib + " /ENTRY:main";
            if (opts.verbose) {
                std::cout << "Running: " << link_cmd << std::endl;
            }
            int link_result = system(link_cmd.c_str());
            if (link_result != 0) {
                std::cerr << "Error: Linking failed" << std::endl;
                return 1;
            }

            if (opts.verbose) {
                std::cout << "Generated executable: " << exe_file << std::endl;
            }
            std::cout << "Compilation successful: " << exe_file << std::endl;
            return 0;
        }
        
        if (opts.verbose) {
            std::cout << "Pipeline completed successfully" << std::endl;
        }
        
        std::cout << "Compilation successful: " << opts.output_file << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
