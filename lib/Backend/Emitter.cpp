#include "kern/backend/Emitter.h"
#include <cstring>
#include <vector>
#include <unordered_map>
#include <unordered_set>

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
            if (instr.width == 32) {
                // x86-64: writing to 32-bit register auto-zeros upper 32 bits
                out_ << "mov ";
                emitOperand(instr.dst(), 32);
                out_ << ", ";
                if (instr.src1().isStack()) {
                    out_ << sizePrefix(32) << " ";
                }
                emitOperand(instr.src1(), 32);
            } else {
                out_ << "movzx ";
                emitOperand(instr.dst(), 64);
                out_ << ", ";
                if (instr.src1().isStack()) {
                    out_ << sizePrefix(instr.width) << " ";
                }
                emitOperand(instr.src1(), instr.width);
            }
            break;

        case X86Op::MovSX:
            // NASM: movsxd for 32→64, movsx for 8→N and 16→N
            out_ << (instr.width == 32 ? "movsxd " : "movsx ");
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
            if (instr.width == 16) out_ << "cwd";
            else if (instr.width == 32) out_ << "cdq";
            else out_ << "cqo";
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

        case X86Op::Cmovcc:
            out_ << "cmov" << condCodeSuffix(instr.cc) << " ";
            emitOperand(instr.dst(), instr.width);
            out_ << ", ";
            emitOperand(instr.src1(), instr.width);
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

        case X86Op::FloatLoad:
            // movss/movsd xmm, [gpr]  — float load from pointer
            out_ << (instr.width == 32 ? "movss" : "movsd") << " ";
            emitOperand(instr.dst(), instr.width);
            out_ << ", [";
            emitOperand(instr.src1(), 64);
            out_ << "]";
            break;

        case X86Op::FloatStore:
            // movss/movsd [gpr], xmm  — float store to pointer
            out_ << (instr.width == 32 ? "movss" : "movsd") << " [";
            emitOperand(instr.dst(), 64);
            out_ << "], ";
            emitOperand(instr.src1(), instr.width);
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

        // Float <-> Integer conversions
        case X86Op::Cvttsd2si:
        case X86Op::Cvttss2si:
            // cvttsd2si gpr, xmm  /  cvttss2si gpr, xmm
            out_ << x86OpName(instr.op) << " ";
            emitOperand(instr.dst(), instr.width);  // GPR with target width
            out_ << ", ";
            emitOperand(instr.src1(), 128);  // XMM (always full name)
            break;

        case X86Op::Cvtsi2sd:
        case X86Op::Cvtsi2ss:
            // cvtsi2sd xmm, gpr  /  cvtsi2ss xmm, gpr
            out_ << x86OpName(instr.op) << " ";
            emitOperand(instr.dst(), 128);   // XMM dest
            out_ << ", ";
            emitOperand(instr.src1(), instr.width);  // GPR source
            break;

        case X86Op::Cvtsd2ss:
        case X86Op::Cvtss2sd:
            // cvtsd2ss xmm, xmm  /  cvtss2sd xmm, xmm
            out_ << x86OpName(instr.op) << " ";
            emitOperand(instr.dst(), 128);   // XMM dest
            out_ << ", ";
            emitOperand(instr.src1(), 128);  // XMM source
            break;

        case X86Op::Pseudo_ParallelMove:
            emitParallelMove(instr);
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
            if (instr.asm_data.output_count == 0 && instr.asm_data.input_count == 0) {
                // Legacy: emit raw assembly lines directly
                for (uint32_t i = 0; i < instr.asm_data.line_count; ++i) {
                    if (i > 0) out_ << "\n    ";
                    out_.write(instr.asm_data.lines[i], instr.asm_data.line_lengths[i]);
                }
            } else {
                // Extended: substitute $N with resolved register names
                // Operand numbering: outputs first, then inputs
                for (uint32_t i = 0; i < instr.asm_data.line_count; ++i) {
                    if (i > 0) out_ << "\n    ";
                    const char* text = instr.asm_data.lines[i];
                    uint32_t len = instr.asm_data.line_lengths[i];
                    for (uint32_t j = 0; j < len; ++j) {
                        if (text[j] == '$' && j + 1 < len && text[j + 1] >= '0' && text[j + 1] <= '9') {
                            uint32_t idx = text[j + 1] - '0';
                            ++j; // skip digit
                            if (idx < instr.asm_data.output_count) {
                                out_ << physRegName(instr.asm_data.outputs[idx].phys);
                            } else {
                                uint32_t in_idx = idx - instr.asm_data.output_count;
                                if (in_idx < instr.asm_data.input_count) {
                                    out_ << physRegName(instr.asm_data.inputs[in_idx].phys);
                                } else {
                                    out_ << '$' << static_cast<char>('0' + idx);
                                }
                            }
                        } else {
                            out_ << text[j];
                        }
                    }
                }
            }
            break;

        case X86Op::LockCmpxchg:
            // lock cmpxchg [ptr], desired — rax holds expected, result in rax
            if (instr.src1().isStack()) {
                out_ << "mov r11, ";
                emitOperand(instr.src1(), 64);
                out_ << "\n    lock cmpxchg [r11], ";
            } else {
                out_ << "lock cmpxchg [";
                emitOperand(instr.src1(), 64);
                out_ << "], ";
            }
            emitOperand(instr.src2(), 64);
            break;
        case X86Op::LockXadd:
            // lock xadd [ptr], value — old value returned in value reg
            if (instr.src1().isStack()) {
                out_ << "mov r11, ";
                emitOperand(instr.src1(), 64);
                out_ << "\n    lock xadd [r11], ";
            } else {
                out_ << "lock xadd [";
                emitOperand(instr.src1(), 64);
                out_ << "], ";
            }
            emitOperand(instr.src2(), 64);
            break;
        case X86Op::Xchg:
            // xchg [ptr], value — implicitly locked
            if (instr.dst().isStack()) {
                out_ << "mov r11, ";
                emitOperand(instr.dst(), 64);
                out_ << "\n    xchg [r11], ";
            } else {
                out_ << "xchg [";
                emitOperand(instr.dst(), 64);
                out_ << "], ";
            }
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
            if (instr.src1().isStack()) {
                out_ << "mov r11, ";
                emitOperand(instr.src1(), 64);
                out_ << "\n    mov ";
                emitOperand(instr.dst(), instr.width);
                out_ << ", [gs:r11]";
            } else {
                out_ << "mov ";
                emitOperand(instr.dst(), instr.width);
                out_ << ", [gs:";
                emitOperand(instr.src1(), 64);
                out_ << "]";
            }
            break;
        case X86Op::GsStore:
            if (instr.dst().isStack()) {
                out_ << "mov r11, ";
                emitOperand(instr.dst(), 64);
                out_ << "\n    mov [gs:r11], ";
            } else {
                out_ << "mov [gs:";
                emitOperand(instr.dst(), 64);
                out_ << "], ";
            }
            emitOperand(instr.src1(), 64);
            break;
        case X86Op::MovLoadGlobal: {
            // x86 can't do mem-to-mem mov, so if dst is stack, use rax as temp
            auto& dst = instr.dst();
            const char* sp = symPrefix();
            if (dst.isStack()) {
                const char* tmp = (instr.width <= 32) ? "eax" : "rax";
                out_ << "mov " << tmp << ", [rel " << sp << instr.global_label << "]\n";
                out_ << "    mov ";
                emitOperand(dst, instr.width);
                out_ << ", " << tmp;
            } else {
                out_ << "mov ";
                emitOperand(dst, instr.width);
                out_ << ", [rel " << sp << instr.global_label << "]";
            }
            break;
        }
        case X86Op::MovStoreGlobal: {
            auto& src = instr.dst();
            const char* sp = symPrefix();
            if (src.isStack()) {
                const char* tmp = (instr.width <= 32) ? "eax" : "rax";
                out_ << "mov " << tmp << ", ";
                emitOperand(src, instr.width);
                out_ << "\n    mov [rel " << sp << instr.global_label << "], " << tmp;
            } else {
                out_ << "mov [rel " << sp << instr.global_label << "], ";
                emitOperand(src, instr.width);
            }
            break;
        }

        case X86Op::LeaGlobal: {
            const char* sp = symPrefix();
            auto& dst = instr.dst();
            if (dst.isStack()) {
                out_ << "lea rax, [rel " << sp << instr.global_label << "]\n";
                out_ << "    mov ";
                emitOperand(dst, 64);
                out_ << ", rax";
            } else {
                out_ << "lea ";
                emitOperand(dst, 64);
                out_ << ", [rel " << sp << instr.global_label << "]";
            }
            break;
        }

        case X86Op::Bsf:
            out_ << "bsf ";
            emitOperand(instr.dst(), instr.width);
            out_ << ", ";
            emitOperand(instr.src1(), instr.width);
            break;

        case X86Op::Bsr:
            out_ << "bsr ";
            emitOperand(instr.dst(), instr.width);
            out_ << ", ";
            emitOperand(instr.src1(), instr.width);
            break;

        case X86Op::Popcnt:
            out_ << "popcnt ";
            emitOperand(instr.dst(), instr.width);
            out_ << ", ";
            emitOperand(instr.src1(), instr.width);
            break;

        case X86Op::Bswap:
            out_ << "bswap ";
            emitOperand(instr.dst(), instr.width);
            break;

        case X86Op::In:
            // in al/ax/eax, dx  (width determines accumulator size)
            out_ << "in ";
            emitOperand(instr.dst(), instr.width);
            out_ << ", dx";
            break;

        case X86Op::Out:
            // out dx, al/ax/eax
            out_ << "out dx, ";
            emitOperand(instr.src1(), instr.width);
            break;
    }

    out_ << "\n";
}

// ============================================================================
// Parallel Move (cycle-breaking)
// ============================================================================

// Helper: get a unique key for a physical register operand
static uint8_t physKey(PhysReg r) {
    return static_cast<uint8_t>(r);
}

void NASMEmitter::emitParallelMove(const MachInstr& instr) {
    // Collect dst→src pairs (only physical registers)
    struct MovePair {
        MachOperand dst;
        MachOperand src;
    };
    std::vector<MovePair> moves;
    for (uint8_t i = 0; i + 1 < instr.operand_count; i += 2) {
        moves.push_back({instr.operand(i), instr.operand(i + 1)});
    }

    if (moves.empty()) return;

    // Build adjacency: dst_reg → index in moves
    // We need to detect cycles among physical register moves
    std::unordered_map<uint8_t, uint32_t> dst_map; // phys_key → move index
    for (uint32_t i = 0; i < moves.size(); ++i) {
        if (moves[i].dst.isPhysical()) {
            dst_map[physKey(moves[i].dst.phys)] = i;
        }
    }

    // Track which moves have been emitted
    std::vector<bool> emitted(moves.size(), false);

    // Pass 1: emit non-cyclic moves via topological ordering
    // A move is "ready" if its dst is not a src of any un-emitted move
    bool changed = true;
    while (changed) {
        changed = false;
        for (uint32_t i = 0; i < moves.size(); ++i) {
            if (emitted[i]) continue;
            // Check if this move's dst register is used as a src by another un-emitted move
            bool blocked = false;
            if (moves[i].dst.isPhysical()) {
                uint8_t dk = physKey(moves[i].dst.phys);
                for (uint32_t j = 0; j < moves.size(); ++j) {
                    if (j == i || emitted[j]) continue;
                    if (moves[j].src.isPhysical() && physKey(moves[j].src.phys) == dk) {
                        blocked = true;
                        break;
                    }
                }
            }
            if (!blocked) {
                // Safe to emit
                out_ << "    mov ";
                emitOperand(moves[i].dst, 64);
                out_ << ", ";
                emitOperand(moves[i].src, 64);
                out_ << "\n";
                emitted[i] = true;
                changed = true;
            }
        }
    }

    // Pass 2: remaining moves form cycles — break with xchg / scratch r11
    for (uint32_t i = 0; i < moves.size(); ++i) {
        if (emitted[i]) continue;

        // Find the cycle starting from move i
        std::vector<uint32_t> cycle;
        uint32_t cur = i;
        while (cur < moves.size() && !emitted[cur]) {
            cycle.push_back(cur);
            emitted[cur] = true;
            // Follow: the src of cur is the dst of the next move in the cycle
            if (!moves[cur].src.isPhysical()) break;
            auto it = dst_map.find(physKey(moves[cur].src.phys));
            if (it == dst_map.end() || emitted[it->second]) break;
            cur = it->second;
        }

        if (cycle.size() == 2) {
            // 2-cycle: use xchg
            out_ << "    xchg ";
            emitOperand(moves[cycle[0]].dst, 64);
            out_ << ", ";
            emitOperand(moves[cycle[1]].dst, 64);
            out_ << "\n";
        } else if (cycle.size() > 2) {
            // N-cycle: use r11 as scratch
            // Save first dst to r11, shift all moves, then move r11 to last dst
            out_ << "    mov r11, ";
            emitOperand(moves[cycle[0]].dst, 64);
            out_ << "\n";
            for (uint32_t c = 0; c + 1 < cycle.size(); ++c) {
                out_ << "    mov ";
                emitOperand(moves[cycle[c]].dst, 64);
                out_ << ", ";
                emitOperand(moves[cycle[c]].src, 64);
                out_ << "\n";
            }
            out_ << "    mov ";
            emitOperand(moves[cycle.back()].dst, 64);
            out_ << ", r11\n";
        } else if (cycle.size() == 1) {
            // Single un-emitted move (src isn't physical or no cycle) — just emit
            out_ << "    mov ";
            emitOperand(moves[cycle[0]].dst, 64);
            out_ << ", ";
            emitOperand(moves[cycle[0]].src, 64);
            out_ << "\n";
        }
    }
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
        // Save all 16 XMM registers (256 bytes, unaligned store)
        out_ << "    sub rsp, 256\n";
        for (int i = 0; i < 16; ++i) {
            out_ << "    movdqu [rsp+" << (i * 16) << "], xmm" << i << "\n";
        }
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
        // Interrupt handler: restore all XMM + GPRs + iretq
        out_ << "    mov rsp, rbp\n";
        // Restore 16 XMM registers
        for (int i = 0; i < 16; ++i) {
            out_ << "    movdqu xmm" << i << ", [rsp+" << (i * 16) << "]\n";
        }
        out_ << "    add rsp, 256\n";
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
    if (fn.is_intrinsic || fn.is_extern) return;

    // Emit custom section directive if specified
    if (!fn.section_name.empty()) {
        out_ << "section " << fn.section_name << "\n";
    }

    // Function label: @link_name overrides, otherwise mangled if module context exists
    if (!fn.link_name.empty()) {
        out_ << symPrefix() << fn.link_name << ":\n";
    } else if (!module_name_.empty() && fn.name != "main") {
        out_ << symPrefix() << module_name_ << "__" << fn.name << ":\n";
    } else {
        out_ << symPrefix() << fn.name << ":\n";
    }

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

    // Emit extern declarations for extern globals
    for (uint32_t i = 0; i < global_count; ++i) {
        const auto& g = globals[i];
        if (g.kind != GlobalData::Variable || !g.variable.is_extern) continue;
        std::string sym;
        if (!g.variable.link_name.empty()) {
            sym = std::string(symPrefix()) + std::string(g.variable.link_name);
        } else {
            sym = std::string(symPrefix()) + std::string(g.label);
        }
        out_ << "extern " << sym << "\n";
    }

    // .rodata: string literals, float constants, immutable globals (without custom section)
    bool has_rodata = false;
    for (uint32_t i = 0; i < global_count; ++i) {
        const auto& g = globals[i];
        if (g.kind == GlobalData::Variable && g.variable.is_extern) continue;
        if (g.kind == GlobalData::Variable && g.variable.is_mutable) continue;
        if (g.kind == GlobalData::Variable && !g.variable.section_name.empty()) continue;
        if (!has_rodata) { out_ << "section .rodata\n"; has_rodata = true; }

        if (g.kind == GlobalData::StringLit) {
            out_ << g.label << ":\n    db ";
            for (uint32_t j = 0; j < g.string_lit.length; ++j) {
                if (j > 0) out_ << ", ";
                out_ << static_cast<int>(static_cast<uint8_t>(g.string_lit.data[j]));
            }
            out_ << "\n";
        } else if (g.kind == GlobalData::FloatConst) {
            out_ << g.label << ":\n";
            if (g.float_const.is_f32) {
                uint32_t bits;
                float f = static_cast<float>(g.float_const.value);
                std::memcpy(&bits, &f, sizeof(bits));
                out_ << "    dd 0x" << std::hex << bits << std::dec << "\n";
            } else {
                uint64_t bits;
                std::memcpy(&bits, &g.float_const.value, sizeof(bits));
                out_ << "    dq 0x" << std::hex << bits << std::dec << "\n";
            }
        } else if (g.kind == GlobalData::Variable) {
            // Immutable global variable → .rodata
            out_ << symPrefix() << g.label << ":\n";
            emitGlobalVarDirective(g.variable);
        } else if (g.kind == GlobalData::VTable) {
            // VTable: array of function pointer labels
            out_ << "align 8\n";
            out_ << symPrefix() << g.label << ":\n";
            for (uint32_t j = 0; j < g.vtable.method_count; ++j) {
                auto fn_label = g.vtable.fn_labels[j];
                // Mangle: symPrefix + module__fn_name (or just symPrefix + fn_name)
                if (!module_name_.empty() &&
                    fn_label.find("__") == std::string_view::npos &&
                    fn_label != "main") {
                    out_ << "    dq " << symPrefix() << module_name_ << "__" << fn_label << "\n";
                } else {
                    out_ << "    dq " << symPrefix() << fn_label << "\n";
                }
            }
        }
    }
    if (has_rodata) out_ << "\n";

    // .data: mutable globals with non-zero initializer (without custom section)
    bool has_data = false;
    for (uint32_t i = 0; i < global_count; ++i) {
        const auto& g = globals[i];
        if (g.kind != GlobalData::Variable || !g.variable.is_mutable) continue;
        if (g.variable.is_extern) continue;
        if (!g.variable.section_name.empty()) continue;
        bool has_init = g.variable.init_value != 0 || g.variable.array_count > 0 ||
                        g.variable.init_byte_count > 0;
        if (!has_init) continue;  // zero-init goes to .bss
        if (!has_data) { out_ << "section .data\n"; has_data = true; }
        out_ << symPrefix() << g.label << ":\n";
        emitGlobalVarDirective(g.variable);
    }
    if (has_data) out_ << "\n";

    // .bss: mutable globals with zero initializer (without custom section)
    bool has_bss = false;
    for (uint32_t i = 0; i < global_count; ++i) {
        const auto& g = globals[i];
        if (g.kind != GlobalData::Variable || !g.variable.is_mutable) continue;
        if (g.variable.is_extern) continue;
        if (!g.variable.section_name.empty()) continue;
        if (g.variable.init_value != 0 || g.variable.array_count > 0 ||
            g.variable.init_byte_count > 0) continue;
        if (!has_bss) { out_ << "section .bss\n"; has_bss = true; }
        // For structs, init_byte_count may carry the full size
        uint32_t bss_sz = g.variable.init_byte_count > 0 ? g.variable.init_byte_count
                         : static_cast<uint32_t>(g.variable.size);
        out_ << symPrefix() << g.label << ": resb " << bss_sz << "\n";
    }
    if (has_bss) out_ << "\n";

    // Custom sections: globals with @section("name")
    for (uint32_t i = 0; i < global_count; ++i) {
        const auto& g = globals[i];
        if (g.kind != GlobalData::Variable) continue;
        if (g.variable.is_extern) continue;
        if (g.variable.section_name.empty()) continue;
        out_ << "section " << g.variable.section_name << "\n";
        out_ << symPrefix() << g.label << ":\n";
        bool is_zero = (g.variable.init_value == 0 && g.variable.array_count == 0 &&
                        g.variable.init_byte_count == 0);
        if (is_zero) {
            uint32_t sz = g.variable.init_byte_count > 0 ? g.variable.init_byte_count
                         : static_cast<uint32_t>(g.variable.size);
            out_ << "    resb " << sz << "\n";
        } else {
            emitGlobalVarDirective(g.variable);
        }
        out_ << "\n";
    }
}

void NASMEmitter::emitGlobalVarDirective(const GlobalVariable& var) {
    // Raw byte initializer (struct/float literals)
    if (var.init_bytes && var.init_byte_count > 0) {
        out_ << "    db ";
        for (uint32_t i = 0; i < var.init_byte_count; ++i) {
            if (i > 0) out_ << ", ";
            out_ << static_cast<int>(var.init_bytes[i]);
        }
        out_ << "\n";
        return;
    }
    if (var.array_values && var.array_count > 0) {
        // Array initializer: emit one directive per element
        for (uint32_t i = 0; i < var.array_count; ++i) {
            int64_t v = var.array_values[i];
            switch (var.size) {
                case 1: out_ << "    db " << (v & 0xFF) << "\n"; break;
                case 2: out_ << "    dw " << (v & 0xFFFF) << "\n"; break;
                case 4: out_ << "    dd " << (v & 0xFFFFFFFF) << "\n"; break;
                default: out_ << "    dq " << v << "\n"; break;
            }
        }
        return;
    }
    switch (var.size) {
        case 1: out_ << "    db " << (var.init_value & 0xFF) << "\n"; break;
        case 2: out_ << "    dw " << (var.init_value & 0xFFFF) << "\n"; break;
        case 4: out_ << "    dd " << (var.init_value & 0xFFFFFFFF) << "\n"; break;
        default: out_ << "    dq " << var.init_value << "\n"; break;
    }
}

// ============================================================================
// _start Wrapper
// ============================================================================

void NASMEmitter::emitStartWrapper() {
    const char* sp = symPrefix();
    out_ << "section .text\n";
    out_ << "global " << sp << "start\n\n";
    out_ << sp << "start:\n";
    out_ << "    call " << sp << "main\n";
    out_ << "    mov  rdi, rax\n";
    if (format_ == OutputFormat::Elf64) {
        out_ << "    mov  rax, 60\n";          // Linux x86-64 exit syscall
    } else {
        out_ << "    mov  rax, 0x02000001\n";  // macOS exit syscall
    }
    out_ << "    syscall\n";
}

// ============================================================================
// Module Emission
// ============================================================================

void NASMEmitter::emitModule(const MachModule& mod, const LIRModule& lir_mod,
                              bool freestanding) {
    module_name_ = mod.module_name;
    const char* sp = symPrefix();

    // Emit extern declarations for intrinsic and extern "C" functions
    // (skip if the function is also defined in this module)
    std::unordered_set<std::string_view> defined_names;
    for (uint32_t i = 0; i < mod.fn_count; ++i) {
        if (mod.functions[i].block_count > 0) {
            defined_names.insert(mod.functions[i].name);
        }
    }
    for (uint32_t i = 0; i < mod.fn_count; ++i) {
        if ((mod.functions[i].is_intrinsic || mod.functions[i].is_extern)
            && !defined_names.count(mod.functions[i].name)) {
            out_ << "extern " << sp << mod.functions[i].name << "\n";
        }
    }

    // Emit extern declarations for cross-module imports
    // These labels already include the correct prefix from ISel
    for (uint32_t i = 0; i < mod.extern_label_count; ++i) {
        out_ << "extern " << mod.extern_labels[i] << "\n";
    }
    // Export functions as global symbols
    for (uint32_t i = 0; i < mod.fn_count; ++i) {
        auto& fn = mod.functions[i];
        if (fn.is_intrinsic || fn.is_extern) continue;
        std::string sym;
        if (!fn.link_name.empty()) {
            sym = std::string(sp) + std::string(fn.link_name);
        } else if (!mod.module_name.empty() && fn.name != "main") {
            sym = std::string(sp) + std::string(mod.module_name) + "__" + std::string(fn.name);
        } else {
            sym = std::string(sp) + std::string(fn.name);
        }
        out_ << "global " << sym << "\n";
        if (fn.is_weak) {
            out_ << "weak " << sym << "\n";
        }
    }

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

    // In freestanding mode, export main as global entry
    if (freestanding && has_main) {
        out_ << "global " << sp << "main\n";
    }
}

} // namespace kern
