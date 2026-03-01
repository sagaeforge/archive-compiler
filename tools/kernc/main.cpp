#include "kern/lexer/Lexer.h"
#include "kern/parser/Parser.h"
#include "kern/sema/TypeChecker.h"
#include "kern/sema/PurityChecker.h"
#include "kern/ir/IRBuilder.h"
#include "kern/ir/KernIR.h"
#include "kern/codegen/CodeGen.h"
#include "kern/support/Arena.h"
#include "kern/support/Diagnostic.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <unistd.h>

static void printUsage(const char* prog) {
    std::cerr << "Usage: " << prog << " <input.kern> [options]\n"
              << "Options:\n"
              << "  -o <file>       Output binary name (default: a.out)\n"
              << "  -S              Output assembly only (.asm)\n"
              << "  --dump-tokens   Dump token stream\n"
              << "  --dump-ast      Dump AST\n"
              << "  --dump-ir       Dump IR\n"
              << "  --dump-purity   Dump purity analysis\n"
              << "  --help          Show this help\n";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string input_file;
    std::string output_file = "a.out";
    bool asm_only = false;
    bool dump_tokens = false;
    bool dump_ast = false;
    bool dump_ir = false;
    bool dump_purity = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-o" && i + 1 < argc) {
            output_file = argv[++i];
        } else if (arg == "-S") {
            asm_only = true;
        } else if (arg == "--dump-tokens") {
            dump_tokens = true;
        } else if (arg == "--dump-ast") {
            dump_ast = true;
        } else if (arg == "--dump-ir") {
            dump_ir = true;
        } else if (arg == "--dump-purity") {
            dump_purity = true;
        } else if (arg[0] != '-') {
            input_file = arg;
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            return 1;
        }
    }

    if (input_file.empty()) {
        std::cerr << "error: no input file\n";
        return 1;
    }

    // Read source file
    std::ifstream ifs(input_file);
    if (!ifs) {
        std::cerr << "error: cannot open file '" << input_file << "'\n";
        return 1;
    }
    std::stringstream ss;
    ss << ifs.rdbuf();
    std::string source = ss.str();

    kern::DiagnosticEngine diag;
    kern::Arena arena;

    // --- Lexing ---
    if (dump_tokens) {
        kern::Lexer dump_lexer(source, input_file, diag);
        while (true) {
            kern::Token tok = dump_lexer.nextToken();
            std::cout << kern::tokenKindName(tok.kind)
                      << " '" << tok.text << "'"
                      << " [" << tok.loc.line << ":" << tok.loc.col << "]\n";
            if (tok.kind == kern::TokenKind::Eof) break;
        }
        if (!dump_ast && !dump_ir && !asm_only) return 0;
    }

    // --- Parsing ---
    kern::Lexer lexer(source, input_file, diag);
    kern::Parser parser(lexer, arena, diag);
    kern::Module* mod = parser.parseModule();

    if (diag.hasErrors()) {
        diag.printAll(std::cerr);
        return 1;
    }

    if (dump_ast) {
        kern::dumpAST(mod, std::cout);
        if (!dump_ir && !asm_only) return 0;
    }

    // --- Semantic Analysis ---
    kern::TypeChecker typeChecker(diag);
    typeChecker.check(mod);

    if (diag.hasErrors()) {
        diag.printAll(std::cerr);
        return 1;
    }

    // --- Purity Analysis ---
    kern::PurityChecker purityChecker(diag);
    auto purity_map = purityChecker.analyze(mod);

    // Print warnings (type + purity)
    if (!diag.diagnostics().empty()) {
        diag.printAll(std::cerr);
    }

    if (dump_purity) {
        for (auto& [name, result] : purity_map) {
            std::cout << "fn " << name << ": " << kern::purityName(result.purity);
            if (result.is_recursive) std::cout << " [recursive]";
            std::cout << "\n";
        }
        if (!dump_ir && !asm_only) return 0;
    }

    // --- IR Generation ---
    kern::IRBuilder irBuilder;
    kern::IRModule irMod = irBuilder.build(mod, typeChecker);

    // Apply purity metadata to IR functions
    for (auto& irFn : irMod.functions) {
        auto it = purity_map.find(std::string_view(irFn.name));
        if (it != purity_map.end()) {
            irFn.meta.purity = it->second.purity;
            irFn.meta.is_recursive = it->second.is_recursive;
        }
    }

    if (dump_ir) {
        kern::dumpIR(irMod, std::cout);
        if (!asm_only) return 0;
    }

    // --- Code Generation ---
    std::string asm_file;
    if (asm_only) {
        asm_file = output_file;
        if (asm_file == "a.out") {
            // Replace .kern with .asm, or append .asm
            asm_file = input_file;
            auto dot = asm_file.rfind('.');
            if (dot != std::string::npos) {
                asm_file = asm_file.substr(0, dot);
            }
            asm_file += ".asm";
        }
    } else {
        asm_file = "/tmp/kern_" + std::to_string(getpid()) + ".asm";
    }

    {
        std::ofstream asm_out(asm_file);
        if (!asm_out) {
            std::cerr << "error: cannot create assembly file '" << asm_file << "'\n";
            return 1;
        }

        // Add main wrapper that calls _main and exits via syscall
        // First emit the user's code
        kern::CodeGen codegen(asm_out);
        codegen.emitModule(irMod);

        // Add _start entry point that calls _main and does exit syscall
        // Check if user defined a 'main' function
        bool has_main = false;
        for (const auto& fn : irMod.functions) {
            if (fn.name == "main") { has_main = true; break; }
        }

        if (has_main) {
            asm_out << "global _start\n\n";
            asm_out << "_start:\n";
            asm_out << "    call _main\n";
            asm_out << "    mov  rdi, rax\n";  // exit code = return value of main
            asm_out << "    mov  rax, 0x02000001\n";  // macOS exit syscall
            asm_out << "    syscall\n";
        }
    }

    if (asm_only) {
        std::cout << "Assembly written to " << asm_file << "\n";
        return 0;
    }

    // --- Assemble and Link ---
    std::string obj_file = "/tmp/kern_" + std::to_string(getpid()) + ".o";

    // nasm -f macho64
    std::string nasm_cmd = "nasm -f macho64 " + asm_file + " -o " + obj_file + " 2>&1";
    int nasm_ret = std::system(nasm_cmd.c_str());
    if (nasm_ret != 0) {
        std::cerr << "error: nasm failed\n";
        std::cerr << "  command: " << nasm_cmd << "\n";
        return 1;
    }

    // ld — link with _start entry point
    // macOS requires -lSystem and SDK library path for x86_64 binaries
    std::string sdk_lib_path;
    {
        // Try to find SDK lib path
        FILE* pipe = popen("xcrun --show-sdk-path 2>/dev/null", "r");
        if (pipe) {
            char buf[512];
            if (fgets(buf, sizeof(buf), pipe)) {
                std::string sdk(buf);
                while (!sdk.empty() && (sdk.back() == '\n' || sdk.back() == '\r'))
                    sdk.pop_back();
                sdk_lib_path = sdk + "/usr/lib";
            }
            pclose(pipe);
        }
    }

    std::string ld_cmd = "ld " + obj_file + " -o " + output_file +
                         " -e _start -platform_version macos 14.0.0 14.0.0 -arch x86_64";
    if (!sdk_lib_path.empty()) {
        ld_cmd += " -L" + sdk_lib_path;
    }
    ld_cmd += " -lSystem 2>&1";

    int ld_ret = std::system(ld_cmd.c_str());
    if (ld_ret != 0) {
        std::cerr << "error: linker failed\n";
        std::cerr << "  command: " << ld_cmd << "\n";
        return 1;
    }

    // Clean up temp files
    std::remove(asm_file.c_str());
    std::remove(obj_file.c_str());

    return 0;
}
