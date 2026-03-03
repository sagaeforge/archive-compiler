#include "kern/pipeline/CompilerPipeline.h"
#include "kern/lexer/Lexer.h"
#include "kern/parser/Parser.h"
#include "kern/hir/HIRBuilder.h"
#include "kern/hir/HIRDump.h"
#include "kern/hir/HIRPasses.h"
#include "kern/hir/MonomorphizationPass.h"
#include "kern/lir/LIRBuilder.h"
#include "kern/lir/LIRDump.h"
#include "kern/lir/LIRPass.h"
#include "kern/lir/LIRPasses.h"
#include "kern/backend/X86Backend.h"
#include "kern/backend/Emitter.h"
#include "kern/backend/MachIRDump.h"
#include "kern/support/ModuleResolver.h"

#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <cstdlib>
#include <unistd.h>

namespace kern {

// Resolve @include("path") — search relative to base_dir, then include_paths
static std::string resolveInclude(const std::string& path,
                                  const std::string& base_dir,
                                  const std::vector<std::string>& include_paths) {
    namespace fs = std::filesystem;
    // Try relative to the including file's directory
    fs::path candidate = fs::path(base_dir) / path;
    if (fs::exists(candidate)) return fs::canonical(candidate).string();

    // Try each -I path
    for (const auto& dir : include_paths) {
        candidate = fs::path(dir) / path;
        if (fs::exists(candidate)) return fs::canonical(candidate).string();
    }
    return "";
}

static std::string preprocessIncludesImpl(const std::string& source,
                                          const std::string& base_dir,
                                          const std::vector<std::string>& include_paths,
                                          std::set<std::string>& included,
                                          std::ostream& err, bool& ok) {
    std::string result;
    result.reserve(source.size());
    size_t pos = 0;

    while (pos < source.size()) {
        // Find start of line
        size_t line_start = pos;

        // Skip leading whitespace
        size_t scan = pos;
        while (scan < source.size() && (source[scan] == ' ' || source[scan] == '\t'))
            ++scan;

        // Check for @include("...")
        std::string_view rest(source.data() + scan, source.size() - scan);
        if (rest.size() >= 10 && rest.substr(0, 9) == "@include(") {
            size_t paren = scan + 9;
            // Expect " after (
            if (paren < source.size() && source[paren] == '"') {
                size_t path_start = paren + 1;
                size_t path_end = source.find('"', path_start);
                if (path_end != std::string::npos) {
                    std::string include_path = source.substr(path_start, path_end - path_start);
                    size_t close_paren = path_end + 1;
                    if (close_paren < source.size() && source[close_paren] == ')') {
                        // Skip to end of line
                        size_t eol = source.find('\n', close_paren);
                        if (eol == std::string::npos) eol = source.size();
                        else eol += 1; // include the newline

                        // Resolve path
                        std::string resolved = resolveInclude(include_path, base_dir, include_paths);
                        if (resolved.empty()) {
                            err << "error: included file '" << include_path << "' not found\n";
                            ok = false;
                            pos = eol;
                            continue;
                        }

                        // Circular include check
                        if (included.count(resolved)) {
                            // Already included — skip (include-once semantics)
                            pos = eol;
                            continue;
                        }
                        included.insert(resolved);

                        // Read the file
                        std::ifstream ifs(resolved);
                        if (!ifs) {
                            err << "error: cannot read included file '" << resolved << "'\n";
                            ok = false;
                            pos = eol;
                            continue;
                        }
                        std::stringstream ss;
                        ss << ifs.rdbuf();
                        std::string inc_source = ss.str();

                        // Recursively preprocess included file
                        std::string inc_dir = std::filesystem::path(resolved).parent_path().string();
                        std::string expanded = preprocessIncludesImpl(
                            inc_source, inc_dir, include_paths, included, err, ok);

                        result += expanded;
                        if (!expanded.empty() && expanded.back() != '\n')
                            result += '\n';

                        pos = eol;
                        continue;
                    }
                }
            }
        }

        // Not an @include line — copy through
        size_t eol = source.find('\n', line_start);
        if (eol == std::string::npos) {
            result += source.substr(line_start);
            break;
        }
        result += source.substr(line_start, eol - line_start + 1);
        pos = eol + 1;
    }

    return result;
}

std::string CompilerPipeline::preprocessIncludes(const std::string& source,
                                                  const std::string& base_dir,
                                                  const std::vector<std::string>& include_paths,
                                                  std::ostream& err, bool& ok) {
    ok = true;
    std::set<std::string> included;
    return preprocessIncludesImpl(source, base_dir, include_paths, included, err, ok);
}

CompilerPipeline::CompilerPipeline(CompilationContext& ctx) : ctx_(ctx) {}

Module* CompilerPipeline::parse(const std::string& source,
                                const std::string& filename,
                                const CompileOptions& opts) {
    Lexer lexer(source, filename, ctx_.diag);
    Parser parser(lexer, ctx_.arena, ctx_.diag);

    // Set predefined cfg flags
    if (opts.target == TargetArch::X86_64)
        parser.setCfg("target_arch", "x86_64");
    else if (opts.target == TargetArch::AArch64)
        parser.setCfg("target_arch", "aarch64");

    if (opts.format == OutputFormat::Macho64)
        parser.setCfg("target_os", "macos");
    else if (opts.format == OutputFormat::Elf64)
        parser.setCfg("target_os", "linux");

    if (opts.freestanding)
        parser.setCfg("freestanding");

    // Set user-provided cfg flags
    for (const auto& [key, value] : opts.cfg_flags) {
        if (value.empty())
            parser.setCfg(key);
        else
            parser.setCfg(key, value);
    }

    return parser.parseModule();
}

HIRModule* CompilerPipeline::buildHIR(Module* ast) {
    HIRBuilder builder(ctx_);
    HIRModule* hir = builder.build(ast);
    if (ctx_.diag.hasErrors()) return nullptr;

    // Monomorphization — specialize generic functions before passes
    MonomorphizationPass mono(ctx_);
    hir = mono.run(hir);
    if (!hir || ctx_.diag.hasErrors()) return nullptr;

    // Run HIR passes
    HIRPassManager pm;
    pm.add<PurityAnalysisPass>();
    pm.add<EffectAnalysisPass>();
    pm.add<OwnershipCheckPass>();
    pm.add<TailCallAnalysisPass>();
    pm.add<ConstOverflowPass>();
    pm.add<LossyCastPass>();
    pm.add<BorrowEscapePass>();
    pm.add<MutBorrowAliasPass>();
    pm.add<UnusedBindingPass>();
    pm.run(*hir, ctx_);

    return hir;
}

LIRModule* CompilerPipeline::buildLIR(HIRModule* hir) {
    LIRBuilder builder(ctx_);
    return builder.build(hir);
}

void CompilerPipeline::optimizeLIR(LIRModule* lir) {
    LIRPassManager pm;
    pm.add<ConstFoldPass>();
    pm.add<ConstPropPass>();
    pm.add<ConstFoldPass>();   // second pass catches propagated constants
    pm.add<DeadCodeElimPass>();
    pm.run(*lir, ctx_);
}

MachModule* CompilerPipeline::buildMachIR(LIRModule* lir) {
    X86Backend backend(ctx_, format_);
    MachModule* mach = backend.lower(*lir);
    backend.allocateRegisters(*mach);
    return mach;
}

void CompilerPipeline::emitASM(MachModule* mach, LIRModule* lir,
                                std::ostream& asm_out, bool freestanding) {
    NASMEmitter emitter(asm_out, format_);
    emitter.emitModule(*mach, *lir, freestanding);
}

static const char* nasmFormat(OutputFormat fmt) {
    switch (fmt) {
        case OutputFormat::Elf64:      return "elf64";
        case OutputFormat::FlatBinary: return "bin";
        default:                       return "macho64";
    }
}

int CompilerPipeline::assemble(const std::string& asm_file,
                                const std::string& obj_file,
                                std::ostream& err) {
    std::string cmd = "nasm -f " + std::string(nasmFormat(format_)) +
                      " " + asm_file + " -o " + obj_file + " 2>&1";
    int ret = std::system(cmd.c_str());
    if (ret != 0) {
        err << "error: nasm failed\n";
        err << "  command: " << cmd << "\n";
    }
    return ret;
}

// Append user-specified -L and -l flags to a linker command string
static void appendLibFlags(std::string& cmd, const CompileOptions& opts) {
    for (const auto& lp : opts.lib_paths) cmd += " -L" + lp;
    for (const auto& ln : opts.lib_names) cmd += " -l" + ln;
}

int CompilerPipeline::link(const std::string& obj_file,
                            const std::string& output_file,
                            std::ostream& err,
                            const CompileOptions& opts) {
    if (format_ == OutputFormat::Elf64) {
        std::string cmd = "ld " + obj_file + " -o " + output_file +
                          " -e _start";
        if (!opts.linker_script.empty()) {
            cmd += " -T " + opts.linker_script;
        }
        appendLibFlags(cmd, opts);
        cmd += " -lc 2>&1";
        int ret = std::system(cmd.c_str());
        if (ret != 0) {
            err << "error: linker failed (elf64)\n";
            err << "  command: " << cmd << "\n";
        }
        return ret;
    }

    // Mach-O linking: find macOS SDK library path
    std::string sdk_lib_path;
    {
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

    std::string cmd = "ld " + obj_file + " -o " + output_file +
                      " -e _start -platform_version macos 14.0.0 14.0.0 -arch x86_64";
    if (!sdk_lib_path.empty()) {
        cmd += " -L" + sdk_lib_path;
    }
    if (!opts.linker_script.empty()) {
        cmd += " -T " + opts.linker_script;
    }
    appendLibFlags(cmd, opts);
    cmd += " -lSystem 2>&1";

    int ret = std::system(cmd.c_str());
    if (ret != 0) {
        err << "error: linker failed\n";
        err << "  command: " << cmd << "\n";
    }
    return ret;
}

int CompilerPipeline::linkFreestanding(const std::string& obj_file,
                                        const std::string& output_file,
                                        std::ostream& err,
                                        const CompileOptions& opts) {
    const char* entry = (format_ == OutputFormat::Macho64) ? "_main" : "main";
    std::string cmd = "ld " + obj_file + " -o " + output_file +
                      " -e " + entry;
    if (format_ == OutputFormat::Macho64) {
        cmd += " -platform_version macos 14.0.0 14.0.0 -arch x86_64";
    }
    if (!opts.linker_script.empty()) {
        cmd += " -T " + opts.linker_script;
    }
    appendLibFlags(cmd, opts);
    cmd += " -static 2>&1";

    int ret = std::system(cmd.c_str());
    if (ret != 0) {
        err << "error: linker failed (freestanding)\n";
        err << "  command: " << cmd << "\n";
    }
    return ret;
}

int CompilerPipeline::run(const std::string& source, const CompileOptions& opts,
                           std::ostream& out, std::ostream& err) {
    format_ = opts.format;

    // Preprocess @include directives
    std::string base_dir = std::filesystem::path(opts.input_file).parent_path().string();
    if (base_dir.empty()) base_dir = ".";
    bool inc_ok;
    std::string expanded = preprocessIncludes(source, base_dir, opts.include_paths, err, inc_ok);
    if (!inc_ok) return 1;
    const std::string& src = expanded;

    ctx_.diag.setSource(src);

    // --- Dump tokens (standalone, no AST needed) ---
    if (opts.dump_tokens) {
        Lexer dump_lexer(src, opts.input_file, ctx_.diag);
        while (true) {
            Token tok = dump_lexer.nextToken();
            out << tokenKindName(tok.kind)
                << " '" << tok.text << "'"
                << " [" << tok.loc.line << ":" << tok.loc.col << "]\n";
            if (tok.kind == TokenKind::Eof) break;
        }
        if (!opts.dump_ast && !opts.dump_hir && !opts.dump_lir &&
            !opts.dump_machir && !opts.asm_only) {
            return 0;
        }
    }

    // --- Parse ---
    Module* ast = parse(src, opts.input_file, opts);
    if (ctx_.diag.hasErrors()) {
        ctx_.diag.printAll(err);
        return 1;
    }

    if (opts.dump_ast) {
        dumpAST(ast, out);
        if (!opts.dump_hir && !opts.dump_lir && !opts.dump_machir &&
            !opts.asm_only) {
            return 0;
        }
    }

    // --- HIR ---
    HIRModule* hir = buildHIR(ast);
    if (!hir || ctx_.diag.hasErrors()) {
        ctx_.diag.printAll(err);
        return 1;
    }

    if (opts.dump_hir) {
        dumpHIR(hir, ctx_.types, out);
        if (!opts.dump_lir && !opts.dump_machir && !opts.asm_only) {
            return 0;
        }
    }

    if (opts.dump_purity) {
        // Purity info is in HIR function metadata
        for (uint32_t i = 0; i < hir->fn_count; ++i) {
            auto* fn = hir->functions[i];
            out << "fn " << fn->name << ": "
                << purityName(static_cast<Purity>(fn->purity));
            if (fn->is_recursive) {
                out << (fn->is_tail_recursive ? " [tail-recursive]" : " [recursive]");
            }
            out << "\n";
        }
        if (!opts.dump_lir && !opts.dump_machir && !opts.asm_only) {
            return 0;
        }
    }

    // --- LIR ---
    LIRModule* lir = buildLIR(hir);

    // --- LIR Optimization ---
    optimizeLIR(lir);

    if (opts.dump_lir) {
        dumpLIR(lir, ctx_.types, out);
        if (!opts.dump_machir && !opts.asm_only) {
            return 0;
        }
    }

    // --- MachIR ---
    MachModule* mach = buildMachIR(lir);

    if (opts.dump_machir) {
        dumpMachIR(mach, lir, ctx_.types, out);
        if (!opts.asm_only) {
            return 0;
        }
    }

    // --- Emit NASM ---
    std::string asm_file;
    if (opts.asm_only) {
        asm_file = opts.output_file;
        if (asm_file == "a.out") {
            asm_file = opts.input_file;
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
            err << "error: cannot create assembly file '" << asm_file << "'\n";
            return 1;
        }
        emitASM(mach, lir, asm_out, opts.freestanding);
    }

    if (opts.asm_only) {
        out << "Assembly written to " << asm_file << "\n";
        return 0;
    }

    // --- Assemble and Link ---
    std::string obj_file = "/tmp/kern_" + std::to_string(getpid()) + ".o";

    if (assemble(asm_file, obj_file, err) != 0) {
        return 1;
    }

    if (opts.freestanding) {
        if (linkFreestanding(obj_file, opts.output_file, err, opts) != 0) {
            return 1;
        }
    } else if (link(obj_file, opts.output_file, err, opts) != 0) {
        return 1;
    }

    // Clean up temp files
    std::remove(asm_file.c_str());
    std::remove(obj_file.c_str());

    // Print warnings even on success
    if (ctx_.diag.hasWarnings()) {
        ctx_.diag.printAll(err);
    }

    return 0;
}

int CompilerPipeline::linkMultiple(const std::vector<std::string>& obj_files,
                                    const std::string& output_file,
                                    std::ostream& err,
                                    const CompileOptions& opts) {
    std::string cmd = "ld";
    for (const auto& obj : obj_files) cmd += " " + obj;

    if (format_ == OutputFormat::Elf64) {
        cmd += " -o " + output_file + " -e _start";
        if (!opts.linker_script.empty()) cmd += " -T " + opts.linker_script;
        appendLibFlags(cmd, opts);
        cmd += " -lc 2>&1";
    } else {
        std::string sdk_lib_path;
        {
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
        cmd += " -o " + output_file +
               " -e _start -platform_version macos 14.0.0 14.0.0 -arch x86_64";
        if (!sdk_lib_path.empty()) cmd += " -L" + sdk_lib_path;
        if (!opts.linker_script.empty()) cmd += " -T " + opts.linker_script;
        appendLibFlags(cmd, opts);
        cmd += " -lSystem 2>&1";
    }

    int ret = std::system(cmd.c_str());
    if (ret != 0) {
        err << "error: linker failed\n";
        err << "  command: " << cmd << "\n";
    }
    return ret;
}

int CompilerPipeline::compileToObject(const std::string& source,
                                       const CompileOptions& opts,
                                       std::ostream& /*out*/, std::ostream& err) {
    format_ = opts.format;

    // Preprocess @include directives
    std::string base_dir = std::filesystem::path(opts.input_file).parent_path().string();
    if (base_dir.empty()) base_dir = ".";
    bool inc_ok;
    std::string expanded = preprocessIncludes(source, base_dir, opts.include_paths, err, inc_ok);
    if (!inc_ok) return 1;
    const std::string& src = expanded;

    ctx_.diag.setSource(src);

    // Parse
    Module* ast = parse(src, opts.input_file, opts);
    if (ctx_.diag.hasErrors()) { ctx_.diag.printAll(err); return 1; }

    // HIR
    HIRModule* hir = buildHIR(ast);
    if (!hir || ctx_.diag.hasErrors()) { ctx_.diag.printAll(err); return 1; }

    // LIR
    LIRModule* lir = buildLIR(hir);
    optimizeLIR(lir);

    // MachIR
    MachModule* mach = buildMachIR(lir);

    // Determine output path for the .o file
    std::string obj_file = opts.output_file;
    if (obj_file == "a.out") {
        obj_file = opts.input_file;
        auto dot = obj_file.rfind('.');
        if (dot != std::string::npos) obj_file = obj_file.substr(0, dot);
        obj_file += ".o";
    }

    // Emit ASM to temp file
    std::string asm_file = "/tmp/kern_" + std::to_string(getpid()) + ".asm";
    {
        std::ofstream asm_out(asm_file);
        if (!asm_out) {
            err << "error: cannot create assembly file '" << asm_file << "'\n";
            return 1;
        }
        // compile-only: always freestanding (no _start wrapper)
        emitASM(mach, lir, asm_out, true);
    }

    // Assemble to .o
    if (assemble(asm_file, obj_file, err) != 0) {
        std::remove(asm_file.c_str());
        return 1;
    }

    std::remove(asm_file.c_str());

    if (ctx_.diag.hasWarnings()) ctx_.diag.printAll(err);
    return 0;
}

int CompilerPipeline::linkObjects(const CompileOptions& opts,
                                   std::ostream& /*out*/, std::ostream& err) {
    format_ = opts.format;

    // Collect all .o files: from input_files and object_files
    std::vector<std::string> all_objs;
    for (const auto& f : opts.input_files) all_objs.push_back(f);
    for (const auto& f : opts.object_files) all_objs.push_back(f);

    if (all_objs.empty()) {
        err << "error: no object files to link\n";
        return 1;
    }

    // If not freestanding, generate a _start stub that calls _main and exits
    std::string start_obj;
    if (!opts.freestanding) {
        std::string start_asm = "/tmp/kern_start_" + std::to_string(getpid()) + ".asm";
        start_obj = "/tmp/kern_start_" + std::to_string(getpid()) + ".o";
        {
            std::ofstream sout(start_asm);
            sout << "section .text\n"
                 << "global _start\n"
                 << "extern _main\n"
                 << "_start:\n"
                 << "    call _main\n"
                 << "    mov rdi, rax\n"
                 << "    mov rax, " << (format_ == OutputFormat::Macho64 ? "0x2000001" : "60") << "\n"
                 << "    syscall\n";
        }
        if (assemble(start_asm, start_obj, err) != 0) {
            std::remove(start_asm.c_str());
            return 1;
        }
        std::remove(start_asm.c_str());
        all_objs.push_back(start_obj);
    }

    int ret;
    if (opts.freestanding) {
        if (all_objs.size() == 1) {
            ret = linkFreestanding(all_objs[0], opts.output_file, err, opts);
        } else {
            ret = linkMultiple(all_objs, opts.output_file, err, opts);
        }
    } else {
        ret = linkMultiple(all_objs, opts.output_file, err, opts);
    }

    if (!start_obj.empty()) std::remove(start_obj.c_str());
    return ret;
}

int CompilerPipeline::runMultiFile(const CompileOptions& opts,
                                    std::ostream& /*out*/, std::ostream& err) {
    std::vector<std::string> obj_files;
    std::vector<std::string> asm_files;

    format_ = opts.format;
    for (const auto& input_file : opts.input_files) {
        // Fresh context for each file
        CompilationContext file_ctx;
        CompilerPipeline file_pipeline(file_ctx);
        file_pipeline.format_ = opts.format;

        std::ifstream ifs(input_file);
        if (!ifs) {
            err << "error: cannot open file '" << input_file << "'\n";
            return 1;
        }
        std::stringstream ss;
        ss << ifs.rdbuf();
        std::string source = ss.str();

        // Preprocess @include directives
        std::string base_dir = std::filesystem::path(input_file).parent_path().string();
        if (base_dir.empty()) base_dir = ".";
        bool inc_ok;
        source = preprocessIncludes(source, base_dir, opts.include_paths, err, inc_ok);
        if (!inc_ok) return 1;

        file_ctx.diag.setSource(source);

        Module* ast = file_pipeline.parse(source, input_file, opts);
        if (file_ctx.diag.hasErrors()) { file_ctx.diag.printAll(err); return 1; }

        HIRModule* hir = file_pipeline.buildHIR(ast);
        if (!hir || file_ctx.diag.hasErrors()) { file_ctx.diag.printAll(err); return 1; }

        LIRModule* lir = file_pipeline.buildLIR(hir);
        file_pipeline.optimizeLIR(lir);
        MachModule* mach = file_pipeline.buildMachIR(lir);

        std::string asm_file = "/tmp/kern_" + std::to_string(getpid()) + "_" +
                               std::to_string(obj_files.size()) + ".asm";
        std::string obj_file = "/tmp/kern_" + std::to_string(getpid()) + "_" +
                               std::to_string(obj_files.size()) + ".o";
        {
            std::ofstream asm_out(asm_file);
            if (!asm_out) {
                err << "error: cannot create assembly file '" << asm_file << "'\n";
                return 1;
            }
            file_pipeline.emitASM(mach, lir, asm_out, opts.freestanding);
        }

        if (file_pipeline.assemble(asm_file, obj_file, err) != 0) return 1;
        obj_files.push_back(obj_file);
        asm_files.push_back(asm_file);
    }

    int ret = linkMultiple(obj_files, opts.output_file, err, opts);

    // Clean up temp files
    for (const auto& f : asm_files) std::remove(f.c_str());
    for (const auto& f : obj_files) std::remove(f.c_str());

    return ret;
}

int CompilerPipeline::runModular(const CompileOptions& opts,
                                  std::ostream& /*out*/, std::ostream& err) {
    format_ = opts.format;
    namespace fs = std::filesystem;

    // Phase 0: Build dependency graph
    ModuleResolver resolver(ctx_.diag);

    // Add search paths: entry file's directory + explicit --module-path dirs
    if (!opts.input_file.empty()) {
        auto entry_dir = fs::path(opts.input_file).parent_path();
        if (entry_dir.empty()) entry_dir = ".";
        resolver.addSearchPath(entry_dir.string());
    }
    for (const auto& mp : opts.module_paths) {
        resolver.addSearchPath(mp);
    }

    if (!resolver.buildDependencyGraph(opts.input_files)) {
        ctx_.diag.printAll(err);
        return 1;
    }

    std::vector<std::string> topo_order;
    if (!resolver.topologicalOrder(topo_order)) {
        ctx_.diag.printAll(err);
        return 1;
    }

    // Phase 1: Parse all modules (source must stay alive for string_view lifetime)
    struct ModuleInfo {
        std::string source;
        Module* ast = nullptr;
        std::string module_path;
        std::string file_path;
    };
    std::unordered_map<std::string, ModuleInfo> mod_info;

    // Track pub type names exported by each module (for parser pre-registration)
    std::unordered_map<std::string, std::vector<std::string_view>> mod_struct_names;
    std::unordered_map<std::string, std::vector<std::string_view>> mod_enum_names;
    std::unordered_map<std::string, std::vector<std::string_view>> mod_union_names;

    for (const auto& mod_path : topo_order) {
        auto* rm = resolver.getModule(mod_path);
        if (!rm) continue;

        ModuleInfo mi;
        mi.module_path = rm->module_path;
        mi.file_path = rm->file_path;

        std::ifstream ifs(rm->file_path);
        if (!ifs) {
            err << "error: cannot open file '" << rm->file_path << "'\n";
            return 1;
        }
        std::stringstream ss;
        ss << ifs.rdbuf();
        mi.source = ss.str();

        ctx_.diag.setSource(mi.source);
        Lexer lexer(mi.source, rm->file_path, ctx_.diag);
        Parser parser(lexer, ctx_.arena, ctx_.diag);

        // Pre-register type names from dependency modules (needed for struct literal parsing)
        for (const auto& dep_path : rm->imports) {
            auto sn_it = mod_struct_names.find(dep_path);
            if (sn_it != mod_struct_names.end()) {
                for (auto name : sn_it->second) parser.addKnownStruct(name);
            }
            auto en_it = mod_enum_names.find(dep_path);
            if (en_it != mod_enum_names.end()) {
                for (auto name : en_it->second) parser.addKnownEnum(name);
            }
            auto un_it = mod_union_names.find(dep_path);
            if (un_it != mod_union_names.end()) {
                for (auto name : un_it->second) parser.addKnownUnion(name);
            }
        }

        // Apply cfg flags
        if (opts.target == TargetArch::X86_64)
            parser.setCfg("target_arch", "x86_64");
        else if (opts.target == TargetArch::AArch64)
            parser.setCfg("target_arch", "aarch64");
        if (opts.format == OutputFormat::Macho64)
            parser.setCfg("target_os", "macos");
        else if (opts.format == OutputFormat::Elf64)
            parser.setCfg("target_os", "linux");
        if (opts.freestanding)
            parser.setCfg("freestanding");
        for (const auto& [key, value] : opts.cfg_flags) {
            if (value.empty()) parser.setCfg(key);
            else parser.setCfg(key, value);
        }

        mi.ast = parser.parseModule();

        if (ctx_.diag.hasErrors()) {
            ctx_.diag.printAll(err);
            return 1;
        }

        // Collect pub type names from this module for downstream parsers
        if (mi.ast) {
            std::vector<std::string_view> snames, enames, unames;
            for (uint32_t i = 0; i < mi.ast->struct_count; ++i) {
                if (mi.ast->structs[i]->is_pub)
                    snames.push_back(mi.ast->structs[i]->name);
            }
            for (uint32_t i = 0; i < mi.ast->enum_count; ++i) {
                if (mi.ast->enums[i]->is_pub)
                    enames.push_back(mi.ast->enums[i]->name);
            }
            for (uint32_t i = 0; i < mi.ast->union_count; ++i) {
                if (mi.ast->unions[i]->is_pub)
                    unames.push_back(mi.ast->unions[i]->name);
            }
            if (!snames.empty()) mod_struct_names[mod_path] = std::move(snames);
            if (!enames.empty()) mod_enum_names[mod_path] = std::move(enames);
            if (!unames.empty()) mod_union_names[mod_path] = std::move(unames);
        }

        mod_info[mod_path] = std::move(mi);
    }

    // Phase 2: Register exports in topo order (leaves first)
    // For each module, register its pub symbols so downstream modules can see them.
    // We use a single shared HIRBuilder that accumulates all registered types/fns.
    // Then for each module's actual build, we create a fresh HIRBuilder but
    // pre-seed it with the registered exports from dependencies.

    // Collect which names each module imports
    struct ImportSpec {
        std::string from_module;
        std::vector<std::string_view> names; // specific names, empty = all
    };
    std::unordered_map<std::string, std::vector<ImportSpec>> module_imports;

    for (const auto& mod_path : topo_order) {
        auto it = mod_info.find(mod_path);
        if (it == mod_info.end()) continue;
        auto* ast = it->second.ast;
        if (!ast) continue;

        std::vector<ImportSpec> specs;
        for (uint32_t i = 0; i < ast->import_count; ++i) {
            auto* imp = ast->imports[i];
            ImportSpec is;
            is.from_module = std::string(imp->module_path);
            for (uint32_t j = 0; j < imp->name_count; ++j) {
                is.names.push_back(imp->names[j]);
            }
            specs.push_back(std::move(is));
        }
        module_imports[mod_path] = std::move(specs);
    }

    // Phase 3: Compile each module in topo order
    // Each module gets its own HIRBuilder, pre-seeded with dependency exports.
    // All share the same CompilationContext (Arena, TypeTable, StringPool).
    std::vector<std::string> obj_files;
    std::vector<std::string> asm_files;

    // Track which modules have been registered (their pub symbols)
    std::unordered_map<std::string, Module*> registered_asts;

    for (const auto& mod_path : topo_order) {
        auto it = mod_info.find(mod_path);
        if (it == mod_info.end()) continue;
        auto& mi = it->second;
        if (!mi.ast) continue;

        ctx_.diag.setSource(mi.source);

        // Create a fresh HIRBuilder for this module
        HIRBuilder builder(ctx_);

        // Inject exports from dependency modules
        auto imp_it = module_imports.find(mod_path);
        if (imp_it != module_imports.end()) {
            for (const auto& spec : imp_it->second) {
                auto dep_it = mod_info.find(spec.from_module);
                if (dep_it == mod_info.end()) continue;

                // Register the dependency's pub exports into this builder
                auto dep_mod_name = dep_it->second.ast->module_name;
                if (dep_mod_name.empty()) {
                    dep_mod_name = ctx_.strings.intern(dep_it->second.module_path);
                }
                builder.registerExports(dep_it->second.ast, dep_mod_name);

                // Handle re-exports: if the dependency has `pub import X (a, b)`,
                // register those symbols from X as well
                auto* dep_ast = dep_it->second.ast;
                for (uint32_t i = 0; i < dep_ast->import_count; ++i) {
                    auto* imp = dep_ast->imports[i];
                    if (!imp->is_pub) continue;
                    // This is a pub import — re-export from the original module
                    auto orig_it = mod_info.find(std::string(imp->module_path));
                    if (orig_it == mod_info.end()) continue;
                    auto orig_mod_name = orig_it->second.ast->module_name;
                    if (orig_mod_name.empty()) {
                        orig_mod_name = ctx_.strings.intern(orig_it->second.module_path);
                    }
                    builder.registerExports(orig_it->second.ast, orig_mod_name);
                }
            }
        }

        // Ensure module name is set (from resolver path if not declared in source)
        if (mi.ast->module_name.empty() && !mi.module_path.empty()) {
            mi.ast->module_name = ctx_.strings.intern(mi.module_path);
        }

        // Build HIR
        HIRModule* hir = builder.build(mi.ast);
        if (!hir || ctx_.diag.hasErrors()) {
            ctx_.diag.printAll(err);
            return 1;
        }

        // Monomorphization
        MonomorphizationPass mono(ctx_);
        hir = mono.run(hir);
        if (!hir || ctx_.diag.hasErrors()) {
            ctx_.diag.printAll(err);
            return 1;
        }

        // HIR passes
        HIRPassManager pm;
        pm.add<PurityAnalysisPass>();
        pm.add<EffectAnalysisPass>();
        pm.add<OwnershipCheckPass>();
        pm.add<TailCallAnalysisPass>();
        pm.add<ConstOverflowPass>();
        pm.add<LossyCastPass>();
        pm.add<BorrowEscapePass>();
        pm.add<MutBorrowAliasPass>();
        pm.add<UnusedBindingPass>();
        pm.run(*hir, ctx_);

        if (ctx_.diag.hasErrors()) {
            ctx_.diag.printAll(err);
            return 1;
        }

        // LIR
        LIRBuilder lir_builder(ctx_);
        LIRModule* lir = lir_builder.build(hir);

        // LIR optimization
        LIRPassManager lir_pm;
        lir_pm.add<ConstFoldPass>();
        lir_pm.add<ConstPropPass>();
        lir_pm.add<ConstFoldPass>();
        lir_pm.add<DeadCodeElimPass>();
        lir_pm.run(*lir, ctx_);

        // MachIR
        X86Backend backend(ctx_, format_);
        MachModule* mach = backend.lower(*lir);
        backend.allocateRegisters(*mach);

        // Emit ASM
        std::string asm_file = "/tmp/kern_" + std::to_string(getpid()) + "_" +
                               std::to_string(obj_files.size()) + ".asm";
        std::string obj_file = "/tmp/kern_" + std::to_string(getpid()) + "_" +
                               std::to_string(obj_files.size()) + ".o";
        {
            std::ofstream asm_out(asm_file);
            if (!asm_out) {
                err << "error: cannot create assembly file '" << asm_file << "'\n";
                return 1;
            }
            NASMEmitter emitter(asm_out, format_);
            // Only emit _start wrapper for the entry module (the one with main)
            bool has_main = false;
            for (uint32_t i = 0; i < mi.ast->fn_count; ++i) {
                if (mi.ast->functions[i]->name == "main") {
                    has_main = true;
                    break;
                }
            }
            emitter.emitModule(*mach, *lir, !has_main || opts.freestanding);
        }

        // Assemble
        if (assemble(asm_file, obj_file, err) != 0) return 1;
        obj_files.push_back(obj_file);
        asm_files.push_back(asm_file);

        // Track this module as registered
        registered_asts[mod_path] = mi.ast;
    }

    // Phase 4: Link all .o files
    int ret = linkMultiple(obj_files, opts.output_file, err, opts);

    // Clean up temp files
    for (const auto& f : asm_files) std::remove(f.c_str());
    for (const auto& f : obj_files) std::remove(f.c_str());

    // Print warnings even on success
    if (ctx_.diag.hasWarnings()) {
        ctx_.diag.printAll(err);
    }

    return ret;
}

} // namespace kern
