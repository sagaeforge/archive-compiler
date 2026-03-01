#pragma once
#include "kern/support/CompilationContext.h"
#include "kern/hir/HIR.h"
#include "kern/lir/LIR.h"
#include "kern/backend/MachIR.h"
#include "kern/parser/AST.h"
#include <ostream>
#include <string>

namespace kern {

struct CompileOptions {
    std::string input_file;
    std::string output_file = "a.out";
    bool asm_only = false;
    bool dump_tokens = false;
    bool dump_ast = false;
    bool dump_hir = false;
    bool dump_lir = false;
    bool dump_machir = false;
    bool dump_purity = false;
};

// Orchestrates the full compilation pipeline:
//   Source → Lexer → Parser → AST → HIR → LIR → MachIR → NASM → assemble → link
class CompilerPipeline {
public:
    explicit CompilerPipeline(CompilationContext& ctx);

    // Run the full pipeline. Returns 0 on success, non-zero on error.
    int run(const std::string& source, const CompileOptions& opts,
            std::ostream& out, std::ostream& err);

private:
    CompilationContext& ctx_;

    // Individual stages
    Module* parse(const std::string& source, const std::string& filename);
    HIRModule* buildHIR(Module* ast);
    LIRModule* buildLIR(HIRModule* hir);
    MachModule* buildMachIR(LIRModule* lir);
    void emitASM(MachModule* mach, LIRModule* lir, std::ostream& asm_out);

    int assemble(const std::string& asm_file, const std::string& obj_file,
                 std::ostream& err);
    int link(const std::string& obj_file, const std::string& output_file,
             std::ostream& err);
};

} // namespace kern
