#pragma once
#include "kern/backend/MachIR.h"
#include "kern/backend/TargetBackend.h"
#include "kern/lir/LIR.h"
#include <ostream>
#include <string>
#include <vector>

namespace kern {

class NASMEmitter {
    std::ostream& out_;
    std::string_view module_name_;  // set by emitModule, used for mangling
    OutputFormat format_;           // output format (macho64/elf64/bin)

    // dyn dispatch thunks: generated during emitRodata, emitted in .text
    struct DynThunk {
        std::string thunk_label;   // thunk symbol
        std::string target_label;  // real function symbol
        uint32_t self_size;        // byte size of concrete self type
    };
    std::vector<DynThunk> dyn_thunks_;

public:
    explicit NASMEmitter(std::ostream& out, OutputFormat fmt = OutputFormat::Macho64)
        : out_(out), format_(fmt) {}

    // Symbol prefix: "_" for Mach-O, "" for ELF/flat binary
    const char* symPrefix() const {
        return format_ == OutputFormat::Macho64 ? "_" : "";
    }

    // Emit a complete module: .rodata + .text sections
    // If freestanding=true, omits _start wrapper
    void emitModule(const MachModule& mod, const LIRModule& lir_mod,
                    bool freestanding = false);

    // Emit a single function (with prologue/epilogue)
    void emitFunction(const MachFunction& fn);

    // Emit globals (.rodata section)
    void emitRodata(const GlobalData* globals, uint32_t global_count);

    // Emit _start entry point (calls _main, exits)
    void emitStartWrapper();

private:
    // Emit a single instruction as NASM text
    void emitInstr(const MachInstr& instr);

    // Emit prologue: push rbp, callee-saved, sub rsp
    void emitPrologue(const MachFunction& fn);

    // Emit epilogue: add rsp, callee-saved, pop rbp, ret
    void emitEpilogue(const MachFunction& fn);

    // Format an operand as NASM text
    void emitOperand(const MachOperand& op, uint8_t width);

    // Memory size prefix for width (byte, word, dword, qword)
    static const char* sizePrefix(uint8_t width);

    // Emit db/dw/dd/dq directive for a global variable
    void emitGlobalVarDirective(const GlobalVariable& var);

    // Emit parallel move with cycle-breaking (xchg for 2-cycles, r11 for longer)
    void emitParallelMove(const MachInstr& instr);

    // Emit dyn dispatch thunks collected during emitRodata
    void emitDynThunks();
};

} // namespace kern
