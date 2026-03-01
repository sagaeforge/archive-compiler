#include "kern/pipeline/CompilerPipeline.h"
#include "kern/lexer/Lexer.h"
#include "kern/parser/Parser.h"
#include "kern/hir/HIRBuilder.h"
#include "kern/hir/HIRDump.h"
#include "kern/hir/HIRPasses.h"
#include "kern/lir/LIRBuilder.h"
#include "kern/lir/LIRDump.h"
#include "kern/backend/X86Backend.h"
#include "kern/backend/Emitter.h"
#include "kern/backend/MachIRDump.h"
#include "kern/ir/Metadata.h"

#include <fstream>
#include <cstdlib>
#include <unistd.h>

namespace kern {

CompilerPipeline::CompilerPipeline(CompilationContext& ctx) : ctx_(ctx) {}

Module* CompilerPipeline::parse(const std::string& source,
                                const std::string& filename) {
    Lexer lexer(source, filename, ctx_.diag);
    Parser parser(lexer, ctx_.arena, ctx_.diag);
    return parser.parseModule();
}

HIRModule* CompilerPipeline::buildHIR(Module* ast) {
    HIRBuilder builder(ctx_);
    HIRModule* hir = builder.build(ast);
    if (ctx_.diag.hasErrors()) return nullptr;

    // Run HIR passes
    HIRPassManager pm;
    pm.add<PurityAnalysisPass>();
    pm.add<TailCallAnalysisPass>();
    pm.run(*hir, ctx_);

    return hir;
}

LIRModule* CompilerPipeline::buildLIR(HIRModule* hir) {
    LIRBuilder builder(ctx_);
    return builder.build(hir);
}

MachModule* CompilerPipeline::buildMachIR(LIRModule* lir) {
    X86Backend backend(ctx_);
    MachModule* mach = backend.lower(*lir);
    backend.allocateRegisters(*mach);
    return mach;
}

void CompilerPipeline::emitASM(MachModule* mach, LIRModule* lir,
                                std::ostream& asm_out, bool freestanding) {
    X86Backend backend(ctx_);
    // Use emitter directly since we already have allocated MachIR
    NASMEmitter emitter(asm_out);
    emitter.emitModule(*mach, *lir, freestanding);
}

int CompilerPipeline::assemble(const std::string& asm_file,
                                const std::string& obj_file,
                                std::ostream& err) {
    std::string cmd = "nasm -f macho64 " + asm_file + " -o " + obj_file + " 2>&1";
    int ret = std::system(cmd.c_str());
    if (ret != 0) {
        err << "error: nasm failed\n";
        err << "  command: " << cmd << "\n";
    }
    return ret;
}

int CompilerPipeline::link(const std::string& obj_file,
                            const std::string& output_file,
                            std::ostream& err,
                            const std::string& linker_script) {
    // Find macOS SDK library path
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
    if (!linker_script.empty()) {
        cmd += " -T " + linker_script;
    }
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
                                        const std::string& linker_script) {
    // Freestanding: no _start wrapper, no libc, just raw object → binary
    std::string cmd = "ld " + obj_file + " -o " + output_file +
                      " -e _main -platform_version macos 14.0.0 14.0.0 -arch x86_64";
    if (!linker_script.empty()) {
        cmd += " -T " + linker_script;
    }
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
    ctx_.diag.setSource(source);

    // --- Dump tokens (standalone, no AST needed) ---
    if (opts.dump_tokens) {
        Lexer dump_lexer(source, opts.input_file, ctx_.diag);
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
    Module* ast = parse(source, opts.input_file);
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
        if (linkFreestanding(obj_file, opts.output_file, err,
                             opts.linker_script) != 0) {
            return 1;
        }
    } else if (link(obj_file, opts.output_file, err, opts.linker_script) != 0) {
        return 1;
    }

    // Clean up temp files
    std::remove(asm_file.c_str());
    std::remove(obj_file.c_str());

    return 0;
}

} // namespace kern
