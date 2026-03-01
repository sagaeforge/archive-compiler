#include "kern/pipeline/CompilerPipeline.h"
#include "kern/support/CompilationContext.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

static void printUsage(const char* prog) {
    std::cerr << "Usage: " << prog << " <input.kern> [options]\n"
              << "Options:\n"
              << "  -o <file>       Output binary name (default: a.out)\n"
              << "  -S              Output assembly only (.asm)\n"
              << "  --dump-tokens   Dump token stream\n"
              << "  --dump-ast      Dump AST\n"
              << "  --dump-hir      Dump HIR (typed, desugared)\n"
              << "  --dump-lir      Dump LIR (lowered SSA)\n"
              << "  --dump-machir   Dump MachIR (x86-64 instructions)\n"
              << "  --dump-purity   Dump purity analysis\n"
              << "  --help          Show this help\n";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    kern::CompileOptions opts;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-o" && i + 1 < argc) {
            opts.output_file = argv[++i];
        } else if (arg == "-S") {
            opts.asm_only = true;
        } else if (arg == "--dump-tokens") {
            opts.dump_tokens = true;
        } else if (arg == "--dump-ast") {
            opts.dump_ast = true;
        } else if (arg == "--dump-hir") {
            opts.dump_hir = true;
        } else if (arg == "--dump-lir") {
            opts.dump_lir = true;
        } else if (arg == "--dump-machir") {
            opts.dump_machir = true;
        } else if (arg == "--dump-purity") {
            opts.dump_purity = true;
        } else if (arg[0] != '-') {
            opts.input_file = arg;
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            return 1;
        }
    }

    if (opts.input_file.empty()) {
        std::cerr << "error: no input file\n";
        return 1;
    }

    // Read source file
    std::ifstream ifs(opts.input_file);
    if (!ifs) {
        std::cerr << "error: cannot open file '" << opts.input_file << "'\n";
        return 1;
    }
    std::stringstream ss;
    ss << ifs.rdbuf();
    std::string source = ss.str();

    kern::CompilationContext ctx;
    kern::CompilerPipeline pipeline(ctx);
    return pipeline.run(source, opts, std::cout, std::cerr);
}
