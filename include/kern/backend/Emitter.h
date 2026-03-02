#pragma once
#include "kern/backend/MachIR.h"
#include "kern/lir/LIR.h"
#include <ostream>

namespace kern {

class NASMEmitter {
    std::ostream& out_;

public:
    explicit NASMEmitter(std::ostream& out) : out_(out) {}

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
};

} // namespace kern
