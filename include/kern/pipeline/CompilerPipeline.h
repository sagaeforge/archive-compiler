#pragma once
#include "kern/support/CompilationContext.h"
#include "kern/hir/HIR.h"
#include "kern/lir/LIR.h"
#include "kern/backend/MachIR.h"
#include "kern/backend/TargetBackend.h"
#include "kern/parser/AST.h"
#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace kern {

struct CompileOptions {
    std::string input_file;
    std::vector<std::string> input_files;  // multi-file compilation
    std::vector<std::string> module_paths; // --module-path search dirs
    std::string output_file = "a.out";
    TargetArch target = TargetArch::X86_64;
    OutputFormat format = OutputFormat::Macho64;
    bool asm_only = false;
    bool compile_only = false;             // -c: produce .o, no linking
    bool link_only = false;                // --link: link .o files only
    bool dump_tokens = false;
    bool dump_ast = false;
    bool dump_hir = false;
    bool dump_lir = false;
    bool dump_machir = false;
    bool dump_purity = false;
    bool freestanding = false;
    std::string linker_script;   // --linker-script <file>
    std::vector<std::pair<std::string, std::string>> cfg_flags; // --cfg key=value
    std::vector<std::string> lib_paths;    // -L<path>
    std::vector<std::string> lib_names;    // -l<name>
    std::vector<std::string> object_files; // pre-compiled .o files to link
    std::vector<std::string> include_paths; // -I<path> for @include search
    bool bounds_check = false;             // --bounds-check: runtime array bounds checking
    bool stack_protector = false;          // --stack-protector: stack canary checks
    bool debug_locs = false;               // --debug-locs: emit source location comments in asm
    bool debug_info = false;               // -g: emit DWARF debug info
    bool incremental = false;              // --incremental: cache .o files, skip unchanged modules
    std::string cache_dir = ".kern-cache"; // --cache-dir: build cache directory
    bool pie = false;                      // --pie: position-independent executable
    bool shared = false;                   // --shared: build shared object (.so/.dylib)
    bool relocatable = false;              // -r: produce relocatable object (for kernel modules)
    bool no_red_zone = false;              // --no-red-zone: disable 128-byte red zone (kernel safety)
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
    OutputFormat format_ = OutputFormat::Macho64;
    bool stack_protector_ = false;
    bool debug_locs_ = false;
    bool debug_info_ = false;

    // Individual stages
    Module* parse(const std::string& source, const std::string& filename,
                  const CompileOptions& opts);
    HIRModule* buildHIR(Module* ast);
    LIRModule* buildLIR(HIRModule* hir, const CompileOptions& opts);
    void optimizeLIR(LIRModule* lir);
    MachModule* buildMachIR(LIRModule* lir);
    void emitASM(MachModule* mach, LIRModule* lir, std::ostream& asm_out,
                 bool freestanding = false);

    int assemble(const std::string& asm_file, const std::string& obj_file,
                 std::ostream& err);
    int link(const std::string& obj_file, const std::string& output_file,
             std::ostream& err, const CompileOptions& opts);
    int linkMultiple(const std::vector<std::string>& obj_files,
                     const std::string& output_file,
                     std::ostream& err, const CompileOptions& opts);
    int linkFreestanding(const std::string& obj_file, const std::string& output_file,
                         std::ostream& err, const CompileOptions& opts);

public:
    // Compile source to object file only (no linking). -c flag.
    int compileToObject(const std::string& source, const CompileOptions& opts,
                        std::ostream& out, std::ostream& err);

    // Link pre-compiled object files only. --link flag.
    int linkObjects(const CompileOptions& opts,
                    std::ostream& out, std::ostream& err);

    // Multi-file compilation: compile each file to .o, then link
    int runMultiFile(const CompileOptions& opts,
                     std::ostream& out, std::ostream& err);

    // Modular compilation: resolve imports, topo sort, shared context
    int runModular(const CompileOptions& opts,
                   std::ostream& out, std::ostream& err);

    // Preprocess @include("path") directives (textual inclusion).
    // Returns expanded source. Reports errors to err. Sets ok=false on failure.
    static std::string preprocessIncludes(const std::string& source,
                                          const std::string& base_dir,
                                          const std::vector<std::string>& include_paths,
                                          std::ostream& err, bool& ok);
};

} // namespace kern
