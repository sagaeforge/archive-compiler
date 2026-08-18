#include "kern/backend/PeepholeOptimizer.h"
#include <vector>

namespace kern {

static bool samePhysReg(const MachOperand& a, const MachOperand& b) {
    return a.isPhysical() && b.isPhysical() && a.phys == b.phys;
}

static bool sameStack(const MachOperand& a, const MachOperand& b) {
    return a.isStack() && b.isStack() && a.stack_offset == b.stack_offset;
}

static void optimizeBlock(MachBlock& block) {
    if (block.instr_count == 0) return;

    auto* instrs = block.instrs;
    uint32_t count = block.instr_count;
    std::vector<bool> dead(count, false);

    for (uint32_t i = 0; i < count; ++i) {
        auto& instr = instrs[i];

        // 1. Redundant mov: mov rX, rX → remove
        if (instr.op == X86Op::Mov &&
            samePhysReg(instr.dst(), instr.src1())) {
            dead[i] = true;
            continue;
        }

        // 2. Redundant float mov: movss/movsd xmmN, xmmN → remove
        if ((instr.op == X86Op::Movss || instr.op == X86Op::Movsd) &&
            samePhysReg(instr.dst(), instr.src1())) {
            dead[i] = true;
            continue;
        }

        // 3. Add/Sub 0: add rX, 0 / sub rX, 0 → remove
        if ((instr.op == X86Op::Add || instr.op == X86Op::Sub) &&
            instr.src1().isImm() && instr.src1().imm == 0) {
            dead[i] = true;
            continue;
        }

        // 4. IMul by 1: imul rX, 1 → remove
        if (instr.op == X86Op::IMul &&
            instr.src1().isImm() && instr.src1().imm == 1) {
            dead[i] = true;
            continue;
        }

        // 5. Mov 0 → xor (smaller encoding, clears FLAGS)
        //    Only when the next instruction doesn't read FLAGS.
        if (instr.op == X86Op::Mov &&
            instr.src1().isImm() && instr.src1().imm == 0 &&
            instr.dst().isPhysical() && isGPR(instr.dst().phys)) {
            bool next_reads_flags = false;
            if (i + 1 < count) {
                auto next_op = instrs[i + 1].op;
                next_reads_flags = (next_op == X86Op::Setcc ||
                                    next_op == X86Op::Jcc ||
                                    next_op == X86Op::Cmovcc);
            }
            if (!next_reads_flags) {
                instr.op = X86Op::Xor;
                instr.inline_ops[1] = MachOperand::physical(instr.dst().phys);
                instr.width = 32;
            }
            continue;
        }

        // 6. Push/Pop same register → remove both
        if (instr.op == X86Op::Push && i + 1 < count) {
            auto& next = instrs[i + 1];
            if (next.op == X86Op::Pop) {
                MachOperand push_src = instr.operand(0);
                MachOperand pop_dst = next.operand(0);
                if (samePhysReg(push_src, pop_dst)) {
                    dead[i] = true;
                    dead[i + 1] = true;
                    ++i;
                    continue;
                }
                // Push rX; Pop rY → mov rY, rX
                if (push_src.isPhysical() && pop_dst.isPhysical()) {
                    dead[i] = true;
                    next.op = X86Op::Mov;
                    next.operand_count = 2;
                    next.inline_ops[0] = pop_dst;
                    next.inline_ops[1] = push_src;
                    next.width = 64;
                    ++i;
                    continue;
                }
            }
        }

        // 7. Store then load same stack slot:
        //    mov [rbp+off], rX; mov rY, [rbp+off]
        //    → if rX == rY: remove load
        //    → if rX != rY: replace load with mov rY, rX
        if (instr.op == X86Op::Mov &&
            instr.dst().isStack() && instr.src1().isPhysical() &&
            i + 1 < count) {
            auto& next = instrs[i + 1];
            if (next.op == X86Op::Mov &&
                next.dst().isPhysical() && next.src1().isStack() &&
                sameStack(instr.dst(), next.src1())) {
                if (samePhysReg(instr.src1(), next.dst())) {
                    dead[i + 1] = true;
                } else {
                    next.inline_ops[1] = instr.src1();
                }
                ++i;
                continue;
            }
        }

        // 8. Nop removal
        if (instr.op == X86Op::Nop) {
            dead[i] = true;
            continue;
        }
    }

    // Compact: shift live instructions down
    uint32_t write = 0;
    for (uint32_t read = 0; read < count; ++read) {
        if (!dead[read]) {
            if (write != read) {
                instrs[write] = instrs[read];
            }
            ++write;
        }
    }
    block.instr_count = write;
}

void peepholeOptimize(MachModule& mod) {
    for (uint32_t fi = 0; fi < mod.fn_count; ++fi) {
        auto& fn = mod.functions[fi];
        if (fn.is_naked) continue;
        for (uint32_t bi = 0; bi < fn.block_count; ++bi) {
            optimizeBlock(fn.blocks[bi]);
        }
    }
}

} // namespace kern
