#include "kern/backend/Emitter.h"
#include <cstring>

namespace kern {

// ============================================================================
// Size Prefix
// ============================================================================

const char* NASMEmitter::sizePrefix(uint8_t width) {
    switch (width) {
        case 8:  return "byte";
        case 16: return "word";
        case 32: return "dword";
        case 64: return "qword";
        default: return "qword";
    }
}

// ============================================================================
// Operand Emission
// ============================================================================

void NASMEmitter::emitOperand(const MachOperand& op, uint8_t width) {
    switch (op.kind) {
        case MachOperand::Reg:
            if (op.is_physical) {
                out_ << physRegName(op.phys, width);
            } else {
                // Should not happen after regalloc
                out_ << "%v" << op.vreg;
            }
            break;
        case MachOperand::Imm:
            out_ << op.imm;
            break;
        case MachOperand::Stack:
            out_ << "[rbp" << (op.stack_offset >= 0 ? "+" : "")
                 << op.stack_offset << "]";
            break;
        case MachOperand::Label:
            out_ << op.label;
            break;
        case MachOperand::None:
            break;
    }
}

// ============================================================================
// Instruction Emission
// ============================================================================

void NASMEmitter::emitInstr(const MachInstr& instr) {
    out_ << "    ";

    switch (instr.op) {
        case X86Op::Mov: {
            out_ << "mov ";
            // If dst is stack, need size prefix
            if (instr.dst().isStack()) {
                out_ << sizePrefix(instr.width) << " ";
                emitOperand(instr.dst(), instr.width);
                out_ << ", ";
                emitOperand(instr.src1(), instr.width);
            } else {
                emitOperand(instr.dst(), instr.width);
                out_ << ", ";
                if (instr.src1().isStack()) {
                    emitOperand(instr.src1(), instr.width);
                } else {
                    emitOperand(instr.src1(), instr.width);
                }
            }
            break;
        }

        case X86Op::MovLoad:
            // mov dst, [src]  — load from memory
            out_ << "mov ";
            emitOperand(instr.dst(), instr.width);
            out_ << ", ";
            if (instr.src1().isStack()) {
                emitOperand(instr.src1(), instr.width);
            } else {
                out_ << sizePrefix(instr.width) << " [";
                emitOperand(instr.src1(), 64);
                out_ << "]";
            }
            break;

        case X86Op::MovStore:
            // mov [dst], src  — store to memory
            out_ << "mov ";
            if (instr.dst().isStack()) {
                out_ << sizePrefix(instr.width) << " ";
                emitOperand(instr.dst(), instr.width);
            } else {
                out_ << sizePrefix(instr.width) << " [";
                emitOperand(instr.dst(), 64);
                out_ << "]";
            }
            out_ << ", ";
            emitOperand(instr.src1(), instr.width);
            break;

        case X86Op::MovZX:
            out_ << "movzx ";
            emitOperand(instr.dst(), 64);
            out_ << ", ";
            if (instr.src1().isStack()) {
                out_ << sizePrefix(instr.width) << " ";
            }
            emitOperand(instr.src1(), instr.width);
            break;

        case X86Op::MovSX:
            out_ << "movsx ";
            emitOperand(instr.dst(), 64);
            out_ << ", ";
            if (instr.src1().isStack()) {
                out_ << sizePrefix(instr.width) << " ";
            }
            emitOperand(instr.src1(), instr.width);
            break;

        case X86Op::Lea:
            out_ << "lea ";
            emitOperand(instr.dst(), 64);
            out_ << ", ";
            if (instr.src1().isLabel()) {
                out_ << "[rel " << instr.src1().label << "]";
            } else if (instr.src1().isPhysical()) {
                out_ << "[";
                emitOperand(instr.src1(), 64);
                out_ << "]";
            } else if (instr.src1().isStack()) {
                // Stack operand already formatted as [rbp+N]
                emitOperand(instr.src1(), 64);
            } else {
                emitOperand(instr.src1(), 64);
            }
            break;

        case X86Op::Push:
            out_ << "push ";
            emitOperand(instr.dst(), 64);
            break;

        case X86Op::Pop:
            out_ << "pop ";
            emitOperand(instr.dst(), 64);
            break;

        case X86Op::Add:
        case X86Op::Sub:
        case X86Op::IMul:
        case X86Op::Xor:
        case X86Op::And:
        case X86Op::Or:
            out_ << x86OpName(instr.op) << " ";
            emitOperand(instr.dst(), instr.width);
            out_ << ", ";
            emitOperand(instr.src1(), instr.width);
            break;

        case X86Op::Shl:
        case X86Op::Shr:
        case X86Op::Sar:
            // x86 shift: op dst, cl
            out_ << x86OpName(instr.op) << " ";
            emitOperand(instr.dst(), instr.width);
            out_ << ", cl";
            break;

        case X86Op::Neg:
        case X86Op::Not:
            out_ << x86OpName(instr.op) << " ";
            emitOperand(instr.dst(), instr.width);
            break;

        case X86Op::IDiv:
            out_ << "idiv ";
            emitOperand(instr.dst(), instr.width);
            break;

        case X86Op::Cqo:
            out_ << "cqo";
            break;

        case X86Op::Cmp:
            out_ << "cmp ";
            emitOperand(instr.dst(), instr.width);
            out_ << ", ";
            emitOperand(instr.src1(), instr.width);
            break;

        case X86Op::Test:
            out_ << "test ";
            emitOperand(instr.dst(), instr.width);
            out_ << ", ";
            emitOperand(instr.src1(), instr.width);
            break;

        case X86Op::Setcc:
            out_ << "set" << condCodeSuffix(instr.cc) << " ";
            emitOperand(instr.dst(), 8);
            break;

        case X86Op::Jmp:
            out_ << "jmp ";
            emitOperand(instr.dst(), 64);
            break;

        case X86Op::Jcc:
            out_ << "j" << condCodeSuffix(instr.cc) << " ";
            emitOperand(instr.dst(), 64);
            break;

        case X86Op::Call:
            out_ << "call ";
            emitOperand(instr.dst(), 64);
            break;

        case X86Op::Ret:
            out_ << "ret";
            break;

        // SSE
        case X86Op::Movss:
        case X86Op::Movsd:
            out_ << x86OpName(instr.op) << " ";
            if (instr.dst().isStack()) {
                out_ << sizePrefix(instr.width) << " ";
            }
            emitOperand(instr.dst(), instr.width);
            out_ << ", ";
            if (instr.src1().isLabel()) {
                // Global data reference: movsd xmm0, [rel _float_0]
                out_ << "[rel " << instr.src1().label << "]";
            } else if (instr.src1().isStack()) {
                emitOperand(instr.src1(), instr.width);
            } else {
                emitOperand(instr.src1(), instr.width);
            }
            break;

        case X86Op::Addss:
        case X86Op::Addsd:
        case X86Op::Subss:
        case X86Op::Subsd:
        case X86Op::Mulss:
        case X86Op::Mulsd:
        case X86Op::Divss:
        case X86Op::Divsd:
            out_ << x86OpName(instr.op) << " ";
            emitOperand(instr.dst(), instr.width);
            out_ << ", ";
            emitOperand(instr.src1(), instr.width);
            break;

        case X86Op::Ucomisd:
        case X86Op::Ucomiss:
            out_ << x86OpName(instr.op) << " ";
            emitOperand(instr.dst(), instr.width);
            out_ << ", ";
            emitOperand(instr.src1(), instr.width);
            break;

        case X86Op::Xorps:
        case X86Op::Xorpd:
            out_ << x86OpName(instr.op) << " ";
            emitOperand(instr.dst(), instr.width);
            out_ << ", ";
            emitOperand(instr.src1(), instr.width);
            break;

        case X86Op::Pseudo_ParallelMove:
            // Expand to sequential moves (TODO: cycle-break)
            for (uint8_t i = 0; i + 1 < instr.operand_count; i += 2) {
                out_ << "    mov ";
                emitOperand(instr.operand(i), 64);
                out_ << ", ";
                emitOperand(instr.operand(i + 1), 64);
                if (i + 2 < instr.operand_count) out_ << "\n";
            }
            return;  // don't add newline

        case X86Op::Pseudo_FrameSetup:
            return;  // handled by prologue

        case X86Op::Pseudo_FrameDestroy:
            // Emit epilogue without ret (used for tail calls)
            return;  // handled in emitFunction loop

        case X86Op::Nop:
            out_ << "nop";
            break;

        case X86Op::InlineAsm:
            // Emit raw assembly lines directly
            for (uint32_t i = 0; i < instr.asm_data.line_count; ++i) {
                if (i > 0) out_ << "\n    ";
                out_.write(instr.asm_data.lines[i], instr.asm_data.line_lengths[i]);
            }
            break;

        case X86Op::LockCmpxchg:
            // lock cmpxchg [ptr], desired — rax holds expected, result in rax
            out_ << "lock cmpxchg [";
            emitOperand(instr.src1(), 64);
            out_ << "], ";
            emitOperand(instr.src2(), 64);
            break;
        case X86Op::LockXadd:
            // lock xadd [ptr], value — old value returned in value reg
            out_ << "lock xadd [";
            emitOperand(instr.src1(), 64);
            out_ << "], ";
            emitOperand(instr.src2(), 64);
            break;
        case X86Op::Xchg:
            // xchg [ptr], value — implicitly locked
            out_ << "xchg [";
            emitOperand(instr.dst(), 64);
            out_ << "], ";
            emitOperand(instr.src1(), 64);
            break;
        case X86Op::Mfence:
            out_ << "mfence";
            break;
        case X86Op::Sfence:
            out_ << "sfence";
            break;
        case X86Op::Lfence:
            out_ << "lfence";
            break;
        case X86Op::GsLoad:
            out_ << "mov ";
            emitOperand(instr.dst(), instr.width);
            out_ << ", [gs:";
            emitOperand(instr.src1(), 64);
            out_ << "]";
            break;
        case X86Op::GsStore:
            out_ << "mov [gs:";
            emitOperand(instr.dst(), 64);
            out_ << "], ";
            emitOperand(instr.src1(), 64);
            break;
    }

    out_ << "\n";
}

// ============================================================================
// Prologue / Epilogue
// ============================================================================

void NASMEmitter::emitPrologue(const MachFunction& fn) {
    if (fn.is_interrupt) {
        // Interrupt handler: save ALL GPRs
        out_ << "    push rax\n";
        out_ << "    push rbx\n";
        out_ << "    push rcx\n";
        out_ << "    push rdx\n";
        out_ << "    push rsi\n";
        out_ << "    push rdi\n";
        out_ << "    push rbp\n";
        out_ << "    push r8\n";
        out_ << "    push r9\n";
        out_ << "    push r10\n";
        out_ << "    push r11\n";
        out_ << "    push r12\n";
        out_ << "    push r13\n";
        out_ << "    push r14\n";
        out_ << "    push r15\n";
        out_ << "    mov rbp, rsp\n";
    } else {
        out_ << "    push rbp\n";
        out_ << "    mov rbp, rsp\n";
    }

    // Push callee-saved registers (skip for interrupt — already saved all)
    if (!fn.is_interrupt) {
        for (uint32_t i = 0; i < NUM_CALLEE_SAVED; ++i) {
            if (fn.callee_saved_used[i]) {
                out_ << "    push " << physRegName(CALLEE_SAVED_GPRS[i]) << "\n";
            }
        }
    }

    // Allocate stack frame
    if (fn.stack_size > 0) {
        out_ << "    sub rsp, " << fn.stack_size << "\n";
    }
}

void NASMEmitter::emitEpilogue(const MachFunction& fn) {
    // Deallocate stack frame
    if (fn.stack_size > 0) {
        out_ << "    add rsp, " << fn.stack_size << "\n";
    }

    if (fn.is_interrupt) {
        // Interrupt handler: restore ALL GPRs + iretq
        out_ << "    mov rsp, rbp\n";
        out_ << "    pop r15\n";
        out_ << "    pop r14\n";
        out_ << "    pop r13\n";
        out_ << "    pop r12\n";
        out_ << "    pop r11\n";
        out_ << "    pop r10\n";
        out_ << "    pop r9\n";
        out_ << "    pop r8\n";
        out_ << "    pop rbp\n";
        out_ << "    pop rdi\n";
        out_ << "    pop rsi\n";
        out_ << "    pop rdx\n";
        out_ << "    pop rcx\n";
        out_ << "    pop rbx\n";
        out_ << "    pop rax\n";
        out_ << "    iretq\n";
    } else {
        // Pop callee-saved registers (reverse order)
        for (int i = NUM_CALLEE_SAVED - 1; i >= 0; --i) {
            if (fn.callee_saved_used[i]) {
                out_ << "    pop " << physRegName(CALLEE_SAVED_GPRS[i]) << "\n";
            }
        }
        out_ << "    pop rbp\n";
        out_ << "    ret\n";
    }
}

// ============================================================================
// Function Emission
// ============================================================================

void NASMEmitter::emitFunction(const MachFunction& fn) {
    if (fn.is_intrinsic) return;

    // Emit custom section directive if specified
    if (!fn.section_name.empty()) {
        out_ << "section " << fn.section_name << "\n";
    }

    out_ << "_" << fn.name << ":\n";

    if (!fn.is_naked) {
        emitPrologue(fn);
    }

    for (uint32_t b = 0; b < fn.block_count; ++b) {
        const auto& block = fn.blocks[b];
        // Emit block label (skip first block, it's the entry)
        if (b > 0) {
            out_ << block.label << ":\n";
        }

        for (uint32_t i = 0; i < block.instr_count; ++i) {
            const auto& instr = block.instrs[i];

            // For naked functions, skip all frame-related pseudo-instructions
            if (fn.is_naked) {
                if (instr.op == X86Op::Ret || instr.op == X86Op::Pseudo_FrameSetup ||
                    instr.op == X86Op::Pseudo_FrameDestroy) {
                    if (instr.op == X86Op::Ret) {
                        out_ << "    ret\n";
                    }
                    continue;
                }
                emitInstr(instr);
                continue;
            }

            // Replace ret with epilogue
            if (instr.op == X86Op::Ret) {
                emitEpilogue(fn);
                continue;
            }

            // Emit frame teardown for tail calls (epilogue without ret)
            if (instr.op == X86Op::Pseudo_FrameDestroy) {
                if (fn.stack_size > 0) {
                    out_ << "    add rsp, " << fn.stack_size << "\n";
                }
                for (int j = NUM_CALLEE_SAVED - 1; j >= 0; --j) {
                    if (fn.callee_saved_used[j]) {
                        out_ << "    pop " << physRegName(CALLEE_SAVED_GPRS[j]) << "\n";
                    }
                }
                out_ << "    pop rbp\n";
                continue;
            }

            emitInstr(instr);
        }
    }

    out_ << "\n";
}

// ============================================================================
// .rodata Emission
// ============================================================================

void NASMEmitter::emitRodata(const GlobalData* globals, uint32_t global_count) {
    if (global_count == 0) return;

    out_ << "section .rodata\n";

    for (uint32_t i = 0; i < global_count; ++i) {
        const auto& g = globals[i];
        out_ << g.label << ":\n";

        if (g.kind == GlobalData::StringLit) {
            out_ << "    db ";
            for (uint32_t j = 0; j < g.string_lit.length; ++j) {
                if (j > 0) out_ << ", ";
                out_ << static_cast<int>(static_cast<uint8_t>(g.string_lit.data[j]));
            }
            out_ << "\n";
        } else {
            // Float constant
            if (g.float_const.is_f32) {
                // dd as 32-bit float
                uint32_t bits;
                float f = static_cast<float>(g.float_const.value);
                std::memcpy(&bits, &f, sizeof(bits));
                out_ << "    dd 0x" << std::hex << bits << std::dec << "\n";
            } else {
                // dq as 64-bit double
                uint64_t bits;
                std::memcpy(&bits, &g.float_const.value, sizeof(bits));
                out_ << "    dq 0x" << std::hex << bits << std::dec << "\n";
            }
        }
    }

    out_ << "\n";
}

// ============================================================================
// _start Wrapper
// ============================================================================

void NASMEmitter::emitStartWrapper() {
    out_ << "section .text\n";
    out_ << "global _start\n\n";
    out_ << "_start:\n";
    out_ << "    call _main\n";
    out_ << "    mov  rdi, rax\n";
    out_ << "    mov  rax, 0x02000001\n";  // macOS exit syscall
    out_ << "    syscall\n";
}

// ============================================================================
// Module Emission
// ============================================================================

void NASMEmitter::emitModule(const MachModule& mod, const LIRModule& lir_mod,
                              bool freestanding) {
    // .rodata first
    emitRodata(lir_mod.globals, lir_mod.global_count);

    // .text section
    out_ << "section .text\n\n";

    bool has_main = false;
    for (uint32_t i = 0; i < mod.fn_count; ++i) {
        emitFunction(mod.functions[i]);
        if (mod.functions[i].name == "main") has_main = true;
    }

    // _start wrapper (omitted in freestanding mode)
    if (has_main && !freestanding) {
        emitStartWrapper();
    }

    // In freestanding mode, export _main as global entry
    if (freestanding && has_main) {
        out_ << "global _main\n";
    }
}

} // namespace kern
