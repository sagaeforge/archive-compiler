#include "kern/pipeline/CompilerPipeline.h"
#include "kern/support/CompilationContext.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

static void printUsage(const char* prog) {
    std::cerr << "Usage: " << prog << " <input.kern> [options]\n"
              << "Options:\n"
              << "  -o <file>       Output file name (default: a.out)\n"
              << "  -c              Compile only, produce .o (no linking)\n"
              << "  -S              Output assembly only (.asm)\n"
              << "  --link          Link .o files only (no compilation)\n"
              << "  -I<path>        Add include search path (for @include)\n"
              << "  -L<path>        Add library search path\n"
              << "  -l<name>        Link against library\n"
              << "  -M <dir>        Add module search path\n"
              << "  --module-path <dir>  Add module search path\n"
              << "  --dump-tokens   Dump token stream\n"
              << "  --dump-ast      Dump AST\n"
              << "  --dump-hir      Dump HIR (typed, desugared)\n"
              << "  --dump-lir      Dump LIR (lowered SSA)\n"
              << "  --dump-machir   Dump MachIR (x86-64 instructions)\n"
              << "  --dump-purity   Dump purity analysis\n"
              << "  --freestanding  No _start wrapper, no libc linking\n"
              << "  --linker-script <file>  Use custom linker script\n"
              << "  --target <arch> Target architecture (x86-64, aarch64)\n"
              << "  --format <fmt>  Output format (macho64, elf64, bin)\n"
              << "  --cfg <key[=value]>  Set cfg flag for conditional compilation\n"
              << "  --bounds-check  Enable runtime array bounds checking\n"
              << "  --stack-protector  Enable stack canary protection\n"
              << "  --debug-locs    Emit source locations as comments in asm output\n"
              << "  --help          Show this help\n";
}

// Quick check: does source contain import statements?
static bool hasImports(const std::string& source) {
    // Scan line by line for "import " at start (with optional "pub " prefix)
    size_t pos = 0;
    while (pos < source.size()) {
        // Skip leading whitespace
        while (pos < source.size() && (source[pos] == ' ' || source[pos] == '\t'))
            ++pos;
        if (pos >= source.size()) break;

        // Check for "import " or "pub import "
        std::string_view rest(source.data() + pos, source.size() - pos);
        if (rest.substr(0, 7) == "import " ||
            (rest.size() > 11 && rest.substr(0, 4) == "pub " &&
             rest.substr(4, 7) == "import ")) {
            return true;
        }

        // Advance to next line
        auto nl = source.find('\n', pos);
        if (nl == std::string::npos) break;
        pos = nl + 1;
    }
    return false;
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
        } else if (arg == "-c") {
            opts.compile_only = true;
        } else if (arg == "-S") {
            opts.asm_only = true;
        } else if (arg == "--link") {
            opts.link_only = true;
        } else if (arg.size() > 2 && arg[0] == '-' && arg[1] == 'I') {
            opts.include_paths.push_back(arg.substr(2));
        } else if (arg.size() > 2 && arg[0] == '-' && arg[1] == 'L') {
            opts.lib_paths.push_back(arg.substr(2));
        } else if (arg.size() > 2 && arg[0] == '-' && arg[1] == 'l') {
            opts.lib_names.push_back(arg.substr(2));
        } else if ((arg == "-M" || arg == "--module-path") && i + 1 < argc) {
            opts.module_paths.push_back(argv[++i]);
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
        } else if (arg == "--freestanding") {
            opts.freestanding = true;
        } else if (arg == "--linker-script" && i + 1 < argc) {
            opts.linker_script = argv[++i];
        } else if (arg == "--target" && i + 1 < argc) {
            std::string target = argv[++i];
            if (target == "x86-64" || target == "x86_64") {
                opts.target = kern::TargetArch::X86_64;
            } else if (target == "aarch64" || target == "arm64") {
                opts.target = kern::TargetArch::AArch64;
            } else {
                std::cerr << "error: unknown target '" << target << "'\n";
                return 1;
            }
        } else if (arg == "--format" && i + 1 < argc) {
            std::string fmt = argv[++i];
            if (fmt == "macho64" || fmt == "macho") {
                opts.format = kern::OutputFormat::Macho64;
            } else if (fmt == "elf64" || fmt == "elf") {
                opts.format = kern::OutputFormat::Elf64;
            } else if (fmt == "bin" || fmt == "flat") {
                opts.format = kern::OutputFormat::FlatBinary;
            } else {
                std::cerr << "error: unknown format '" << fmt << "'\n";
                return 1;
            }
        } else if (arg == "--bounds-check") {
            opts.bounds_check = true;
        } else if (arg == "--stack-protector") {
            opts.stack_protector = true;
        } else if (arg == "--debug-locs") {
            opts.debug_locs = true;
        } else if (arg == "--cfg" && i + 1 < argc) {
            std::string cfg = argv[++i];
            auto eq_pos = cfg.find('=');
            if (eq_pos != std::string::npos) {
                opts.cfg_flags.emplace_back(cfg.substr(0, eq_pos), cfg.substr(eq_pos + 1));
            } else {
                opts.cfg_flags.emplace_back(cfg, "");
            }
        } else if (arg[0] != '-') {
            opts.input_files.push_back(arg);
            if (opts.input_file.empty()) opts.input_file = arg;
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            return 1;
        }
    }

    if (opts.input_files.empty()) {
        std::cerr << "error: no input file\n";
        return 1;
    }

    // Separate .o files from .kern files
    {
        std::vector<std::string> kern_files;
        for (const auto& f : opts.input_files) {
            if (f.size() > 2 && f.substr(f.size() - 2) == ".o") {
                opts.object_files.push_back(f);
            } else {
                kern_files.push_back(f);
            }
        }
        opts.input_files = std::move(kern_files);
        if (!opts.input_files.empty())
            opts.input_file = opts.input_files[0];
    }

    kern::CompilationContext ctx;
    kern::CompilerPipeline pipeline(ctx);

    // Link-only mode: just run the linker on .o files
    if (opts.link_only) {
        return pipeline.linkObjects(opts, std::cout, std::cerr);
    }

    // Compile-only mode (-c): produce .o for each .kern file
    if (opts.compile_only) {
        if (opts.input_files.empty()) {
            std::cerr << "error: no source files to compile\n";
            return 1;
        }
        for (size_t i = 0; i < opts.input_files.size(); ++i) {
            std::ifstream ifs(opts.input_files[i]);
            if (!ifs) {
                std::cerr << "error: cannot open file '" << opts.input_files[i] << "'\n";
                return 1;
            }
            std::stringstream ss;
            ss << ifs.rdbuf();

            kern::CompileOptions file_opts = opts;
            file_opts.input_file = opts.input_files[i];
            // If multiple files, derive .o name from each .kern file
            if (opts.input_files.size() > 1 || opts.output_file == "a.out") {
                std::string ofile = opts.input_files[i];
                auto dot = ofile.rfind('.');
                if (dot != std::string::npos) ofile = ofile.substr(0, dot);
                ofile += ".o";
                file_opts.output_file = ofile;
            }

            kern::CompilationContext file_ctx;
            kern::CompilerPipeline file_pipeline(file_ctx);
            int ret = file_pipeline.compileToObject(ss.str(), file_opts, std::cout, std::cerr);
            if (ret != 0) return ret;
        }
        return 0;
    }

    // Multi-file: if multiple files given or single file has imports, use modular
    if (opts.input_files.size() > 1) {
        // Check if any file has imports — if so, use modular compilation
        bool any_imports = false;
        for (const auto& file : opts.input_files) {
            std::ifstream ifs(file);
            if (!ifs) continue;
            std::stringstream ss;
            ss << ifs.rdbuf();
            if (hasImports(ss.str())) {
                any_imports = true;
                break;
            }
        }
        if (any_imports) {
            return pipeline.runModular(opts, std::cout, std::cerr);
        }
        return pipeline.runMultiFile(opts, std::cout, std::cerr);
    }

    // Single file path
    std::ifstream ifs(opts.input_file);
    if (!ifs) {
        std::cerr << "error: cannot open file '" << opts.input_file << "'\n";
        return 1;
    }
    std::stringstream ss;
    ss << ifs.rdbuf();
    std::string source = ss.str();

    // Auto-detect imports in single file → switch to modular
    if (hasImports(source)) {
        return pipeline.runModular(opts, std::cout, std::cerr);
    }

    return pipeline.run(source, opts, std::cout, std::cerr);
}
