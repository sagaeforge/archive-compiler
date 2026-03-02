#include "kern/backend/RegisterAllocator.h"
#include <algorithm>
#include <cassert>

namespace kern {

// ============================================================================
// Global instruction numbering
// ============================================================================

std::vector<RegisterAllocator::InstrIndex>
RegisterAllocator::buildNumbering(const MachFunction& fn) {
    std::vector<InstrIndex> numbering;
    uint32_t global = 0;
    for (uint32_t b = 0; b < fn.block_count; ++b) {
        for (uint32_t i = 0; i < fn.blocks[b].instr_count; ++i) {
            numbering.push_back({b, i, global});
            global++;
        }
    }
    return numbering;
}

// ============================================================================
// Liveness Analysis → Live Intervals
// ============================================================================

static void recordVRegUse(std::unordered_map<uint32_t, LiveInterval>& intervals,
                           uint32_t vreg, uint32_t idx, bool is_float) {
    auto it = intervals.find(vreg);
    if (it == intervals.end()) {
        LiveInterval li;
        li.vreg = vreg;
        li.start = idx;
        li.end = idx;
        li.is_float = is_float;
        intervals[vreg] = li;
    } else {
        if (idx < it->second.start) it->second.start = idx;
        if (idx > it->second.end) it->second.end = idx;
    }
}

static void recordPhysUse(std::unordered_map<uint32_t, LiveInterval>& intervals,
                           PhysReg reg, uint32_t idx) {
    // We use a high vreg range (10000+) for physical registers to track constraints
    uint32_t pseudo_vreg = 10000 + static_cast<uint32_t>(reg);
    auto it = intervals.find(pseudo_vreg);
    if (it == intervals.end()) {
        LiveInterval li;
        li.vreg = pseudo_vreg;
        li.start = idx;
        li.end = idx;
        li.is_fixed = true;
        li.hint = reg;
        li.is_float = isXMM(reg);
        intervals[pseudo_vreg] = li;
    } else {
        if (idx < it->second.start) it->second.start = idx;
        if (idx > it->second.end) it->second.end = idx;
    }
}

static bool isSSEOp(X86Op op) {
    switch (op) {
        case X86Op::Movss: case X86Op::Movsd:
        case X86Op::Addss: case X86Op::Addsd:
        case X86Op::Subss: case X86Op::Subsd:
        case X86Op::Mulss: case X86Op::Mulsd:
        case X86Op::Divss: case X86Op::Divsd:
        case X86Op::Ucomisd: case X86Op::Ucomiss:
        case X86Op::Xorps: case X86Op::Xorpd:
            return true;
        default:
            return false;
    }
}

static void scanOperandDefs(const MachInstr& instr, uint32_t idx,
                             std::unordered_map<uint32_t, LiveInterval>& intervals) {
    // MovStore/FloatStore have no def — both operands are uses
    if (instr.op == X86Op::MovStore || instr.op == X86Op::FloatStore) return;

    // FloatLoad: dst is SSE (XMM), src is GPR
    bool is_sse = isSSEOp(instr.op) || instr.op == X86Op::FloatLoad;

    // For most instructions, operand[0] is the destination (def)
    if (instr.operand_count > 0) {
        const auto& op = instr.operand(0);
        if (op.isVirtual()) {
            recordVRegUse(intervals, op.vreg, idx, is_sse);
        } else if (op.isPhysical()) {
            recordPhysUse(intervals, op.phys, idx);
        }
    }
}

static void scanOperandUses(const MachInstr& instr, uint32_t idx,
                              std::unordered_map<uint32_t, LiveInterval>& intervals) {
    // Operands 1+ are sources (uses) for most instructions
    uint8_t start = 1;

    // Some instructions have all operands as uses (cmp, test, push, call, jmp, jcc)
    if (instr.op == X86Op::Cmp || instr.op == X86Op::Test ||
        instr.op == X86Op::Push || instr.op == X86Op::Call ||
        instr.op == X86Op::Jmp || instr.op == X86Op::Jcc ||
        instr.op == X86Op::Ucomisd || instr.op == X86Op::Ucomiss ||
        instr.op == X86Op::MovStore || instr.op == X86Op::FloatStore) {
        start = 0;  // MovStore: both ptr and val are uses
    }
    // Setcc: operand[0] is dst (def)
    // Ret: no explicit operands
    // IDiv: operand[0] is src (divisor) — special case
    if (instr.op == X86Op::IDiv || instr.op == X86Op::Neg ||
        instr.op == X86Op::Not) {
        start = 0;  // all operands are use+def for destructive ops
    }

    bool is_sse = isSSEOp(instr.op);

    // FloatStore: operand[0]=ptr(GPR), operand[1]=value(XMM) — mixed registers
    if (instr.op == X86Op::FloatStore) {
        for (uint8_t i = start; i < instr.operand_count; ++i) {
            const auto& op = instr.operand(i);
            bool op_sse = (i == 1);  // value operand is XMM
            if (op.isVirtual()) {
                recordVRegUse(intervals, op.vreg, idx, op_sse);
            } else if (op.isPhysical()) {
                recordPhysUse(intervals, op.phys, idx);
            }
        }
    } else {
        for (uint8_t i = start; i < instr.operand_count; ++i) {
            const auto& op = instr.operand(i);
            if (op.isVirtual()) {
                recordVRegUse(intervals, op.vreg, idx, is_sse);
            } else if (op.isPhysical()) {
                recordPhysUse(intervals, op.phys, idx);
            }
        }
    }

    // ALU ops with 2 operands: dst is both def and use (x86 2-address form)
    if (instr.op == X86Op::Add || instr.op == X86Op::Sub ||
        instr.op == X86Op::IMul || instr.op == X86Op::Xor ||
        instr.op == X86Op::And || instr.op == X86Op::Or ||
        instr.op == X86Op::Addsd || instr.op == X86Op::Subsd ||
        instr.op == X86Op::Mulsd || instr.op == X86Op::Divsd ||
        instr.op == X86Op::Addss || instr.op == X86Op::Subss ||
        instr.op == X86Op::Mulss || instr.op == X86Op::Divss) {
        if (instr.operand_count > 0) {
            const auto& op = instr.operand(0);
            if (op.isVirtual()) {
                recordVRegUse(intervals, op.vreg, idx, is_sse);
            }
        }
    }

    // Implicit registers for special instructions
    if (instr.op == X86Op::Cqo) {
        recordPhysUse(intervals, PhysReg::RAX, idx);
        recordPhysUse(intervals, PhysReg::RDX, idx);
    }
    if (instr.op == X86Op::IDiv) {
        recordPhysUse(intervals, PhysReg::RAX, idx);
        recordPhysUse(intervals, PhysReg::RDX, idx);
    }
    if (instr.op == X86Op::LockCmpxchg) {
        recordPhysUse(intervals, PhysReg::RAX, idx);
    }
    // Call clobbers all caller-saved registers (GPR and XMM)
    if (instr.op == X86Op::Call) {
        for (uint32_t i = 0; i < NUM_CALLER_SAVED; ++i) {
            recordPhysUse(intervals, CALLER_SAVED_GPRS[i], idx);
        }
        // All XMM registers are caller-saved in System V AMD64
        for (uint32_t i = 0; i < NUM_ALLOCATABLE_XMMS; ++i) {
            recordPhysUse(intervals, ALLOCATABLE_XMMS[i], idx);
        }
    }
}

std::vector<LiveInterval>
RegisterAllocator::computeIntervals(const MachFunction& fn) {
    std::unordered_map<uint32_t, LiveInterval> interval_map;

    // Build label→block index map and block→global idx ranges
    std::unordered_map<std::string_view, uint32_t> label_to_block;
    std::vector<uint32_t> block_start(fn.block_count);
    std::vector<uint32_t> block_end(fn.block_count);

    uint32_t global_idx = 0;
    for (uint32_t b = 0; b < fn.block_count; ++b) {
        label_to_block[fn.blocks[b].label] = b;
        block_start[b] = global_idx;
        for (uint32_t i = 0; i < fn.blocks[b].instr_count; ++i) {
            const auto& instr = fn.blocks[b].instrs[i];
            scanOperandDefs(instr, global_idx, interval_map);
            scanOperandUses(instr, global_idx, interval_map);
            global_idx++;
        }
        block_end[b] = global_idx > 0 ? global_idx - 1 : 0;
    }

    // Detect loop back-edges (jumps from later blocks to earlier blocks)
    // and extend live intervals of vregs that are live across the loop.
    // A back-edge from block B to block H (where H <= B) means
    // any vreg defined before end of B and used within [H..B] must
    // remain live until end of B.
    for (uint32_t b = 0; b < fn.block_count; ++b) {
        for (uint32_t i = 0; i < fn.blocks[b].instr_count; ++i) {
            const auto& instr = fn.blocks[b].instrs[i];
            if (instr.op != X86Op::Jmp && instr.op != X86Op::Jcc) continue;

            // Find target label
            for (uint8_t j = 0; j < instr.operand_count; ++j) {
                const auto& op = instr.operand(j);
                if (op.kind != MachOperand::Kind::Label) continue;

                auto it = label_to_block.find(op.label);
                if (it == label_to_block.end()) continue;

                uint32_t target_block = it->second;
                if (target_block >= b) continue; // forward edge, not a loop

                // Back-edge: target_block < b
                // Extend all intervals that start before the loop header
                // and are used within the loop body to the end of the
                // back-edge block.
                uint32_t loop_start = block_start[target_block];
                uint32_t loop_end = block_end[b];

                for (auto& [_, li] : interval_map) {
                    if (li.start <= loop_start && li.end >= loop_start &&
                        li.end < loop_end) {
                        li.end = loop_end;
                    }
                }
            }
        }
    }

    // Convert map to sorted vector (by start point)
    std::vector<LiveInterval> intervals;
    intervals.reserve(interval_map.size());
    for (auto& [_, li] : interval_map) {
        intervals.push_back(li);
    }
    std::sort(intervals.begin(), intervals.end(),
              [](const auto& a, const auto& b) { return a.start < b.start; });

    return intervals;
}

// ============================================================================
// Linear Scan Allocation
// ============================================================================

static int findGPRIndex(PhysReg reg) {
    for (uint32_t i = 0; i < NUM_ALLOCATABLE_GPRS; ++i) {
        if (ALLOCATABLE_GPRS[i] == reg) return static_cast<int>(i);
    }
    return -1;
}

static int findXMMIndex(PhysReg reg) {
    for (uint32_t i = 0; i < NUM_ALLOCATABLE_XMMS; ++i) {
        if (ALLOCATABLE_XMMS[i] == reg) return static_cast<int>(i);
    }
    return -1;
}

PhysReg RegisterAllocator::findFreeGPR(const RegState& state, PhysReg hint) {
    // Try hint first
    if (hint != PhysReg::NONE) {
        int idx = findGPRIndex(hint);
        if (idx >= 0 && !state.gpr_used[idx]) return hint;
    }
    // Linear search for free register
    for (uint32_t i = 0; i < NUM_ALLOCATABLE_GPRS; ++i) {
        if (!state.gpr_used[i]) return ALLOCATABLE_GPRS[i];
    }
    return PhysReg::NONE;  // spill needed
}

PhysReg RegisterAllocator::findFreeXMM(const RegState& state, PhysReg hint) {
    if (hint != PhysReg::NONE) {
        int idx = findXMMIndex(hint);
        if (idx >= 0 && !state.xmm_used[idx]) return hint;
    }
    for (uint32_t i = 0; i < NUM_ALLOCATABLE_XMMS; ++i) {
        if (!state.xmm_used[i]) return ALLOCATABLE_XMMS[i];
    }
    return PhysReg::NONE;
}

void RegisterAllocator::markCalleeSaved(PhysReg reg, RegAllocation& alloc) {
    for (uint32_t i = 0; i < NUM_CALLEE_SAVED; ++i) {
        if (CALLEE_SAVED_GPRS[i] == reg) {
            alloc.callee_saved_used[i] = true;
            return;
        }
    }
}

RegAllocation
RegisterAllocator::allocate(std::vector<LiveInterval>& intervals,
                            uint32_t struct_alloc_bytes) {
    RegAllocation alloc;

    // Active intervals sorted by end point
    struct Active {
        uint32_t vreg;
        uint32_t end;
        PhysReg reg;
        bool is_float;
    };
    std::vector<Active> active;

    RegState state;
    // Spill slots start below callee-saved pushes + struct alloc area.
    // Callee-saved pushes occupy [rbp-8] .. [rbp - NUM_CALLEE_SAVED*8].
    // We reserve space for all 5 callee-saved registers (even if not all are used)
    // to avoid the chicken-and-egg problem (callee-saved set isn't known until allocation).
    static constexpr int32_t CALLEE_SAVED_AREA = NUM_CALLEE_SAVED * 8;  // 40
    int32_t next_spill_offset = -CALLEE_SAVED_AREA - static_cast<int32_t>(struct_alloc_bytes) - 8;

    for (auto& interval : intervals) {
        // Expire old intervals
        active.erase(
            std::remove_if(active.begin(), active.end(),
                [&](const Active& a) {
                    if (a.end < interval.start) {
                        // Free the register
                        if (a.is_float) {
                            int idx = findXMMIndex(a.reg);
                            if (idx >= 0) state.xmm_used[idx] = false;
                        } else {
                            int idx = findGPRIndex(a.reg);
                            if (idx >= 0) state.gpr_used[idx] = false;
                        }
                        return true;
                    }
                    return false;
                }),
            active.end());

        // Handle fixed (pre-colored) intervals
        if (interval.is_fixed) {
            PhysReg reg = interval.hint;
            alloc.reg_map[interval.vreg] = reg;

            // Evict any active vreg that's using this register
            for (auto it2 = active.begin(); it2 != active.end(); ) {
                if (it2->reg == reg && !alloc.spill_map.count(it2->vreg) &&
                    it2->vreg != interval.vreg) {
                    // Spill the conflicting vreg
                    alloc.reg_map.erase(it2->vreg);
                    alloc.spill_map[it2->vreg] = next_spill_offset;
                    next_spill_offset -= 8;
                    it2 = active.erase(it2);
                } else {
                    ++it2;
                }
            }

            if (interval.is_float) {
                int idx = findXMMIndex(reg);
                if (idx >= 0) state.xmm_used[idx] = true;
            } else {
                int idx = findGPRIndex(reg);
                if (idx >= 0) state.gpr_used[idx] = true;
            }
            active.push_back({interval.vreg, interval.end, reg, interval.is_float});
            markCalleeSaved(reg, alloc);
            continue;
        }

        // Try to allocate a register
        PhysReg reg;
        if (interval.is_float) {
            reg = findFreeXMM(state, interval.hint);
        } else {
            reg = findFreeGPR(state, interval.hint);
        }

        if (reg != PhysReg::NONE) {
            // Allocated successfully
            alloc.reg_map[interval.vreg] = reg;
            if (interval.is_float) {
                int idx = findXMMIndex(reg);
                if (idx >= 0) state.xmm_used[idx] = true;
            } else {
                int idx = findGPRIndex(reg);
                if (idx >= 0) state.gpr_used[idx] = true;
            }
            active.push_back({interval.vreg, interval.end, reg, interval.is_float});
            markCalleeSaved(reg, alloc);
        } else {
            // Spill: assign a stack slot
            alloc.spill_map[interval.vreg] = next_spill_offset;
            next_spill_offset -= 8;
        }
    }

    // Compute total stack size (must be 16-byte aligned)
    uint32_t callee_saved_count = 0;
    for (uint32_t i = 0; i < NUM_CALLEE_SAVED; ++i) {
        if (alloc.callee_saved_used[i]) callee_saved_count++;
    }

    uint32_t spill_size = static_cast<uint32_t>(-next_spill_offset - 8);
    // Stack layout: [callee-saved pushes][spill slots]
    // rbp points to saved rbp, local data is below rbp
    alloc.stack_size = spill_size;

    // Align to 16 bytes considering push rbp + callee-saved pushes
    // Total pushes = 1 (rbp) + callee_saved_count
    // If (total_pushes + stack_size/8) is odd, need 8 more bytes for alignment
    uint32_t total_pushes = 1 + callee_saved_count;
    uint32_t total_slots = total_pushes + (alloc.stack_size / 8);
    if (total_slots % 2 != 0) {
        alloc.stack_size += 8;
    }

    return alloc;
}

// ============================================================================
// Rewrite: Replace VRegs with Physical Regs / Stack Operands
// ============================================================================

static MachOperand rewriteOperand(const MachOperand& op,
                                   const RegAllocation& alloc) {
    if (!op.isVirtual()) return op;

    auto reg_it = alloc.reg_map.find(op.vreg);
    if (reg_it != alloc.reg_map.end()) {
        return MachOperand::precolored(reg_it->second);
    }

    auto spill_it = alloc.spill_map.find(op.vreg);
    if (spill_it != alloc.spill_map.end()) {
        return MachOperand::stack(spill_it->second);
    }

    // Fallback: shouldn't happen in well-formed code
    return op;
}

// Check if an instruction needs fixup for stack operands
static bool needsFixup(const MachInstr& instr) {
    // LEA: dst must be a register (can't be stack)
    if (instr.op == X86Op::Lea && instr.operand_count > 0 &&
        instr.operand(0).isStack()) {
        return true;
    }
    // Neg/Not: single-operand destructive — operand must be register
    if ((instr.op == X86Op::Neg || instr.op == X86Op::Not) &&
        instr.operand_count > 0 && instr.operand(0).isStack()) {
        return true;
    }
    // Setcc: dst must be register (can't setcc to memory)
    if (instr.op == X86Op::Setcc && instr.operand_count > 0 &&
        instr.operand(0).isStack()) {
        return true;
    }
    // IDiv: operand must be register (can't idiv a stack slot directly — well
    // actually x86 allows idiv mem, but our stack slots are [rbp+off] which is
    // memory, so it should work. Keep this for safety if width mismatches.)
    if (instr.op == X86Op::IDiv && instr.operand_count > 0 &&
        instr.operand(0).isStack()) {
        return true;
    }
    // Two-operand instructions: can't have both dst and src as stack
    if (instr.operand_count >= 2 && instr.operand(0).isStack() &&
        instr.operand(1).isStack()) {
        return true;
    }
    // MovLoad/FloatLoad: load from [src] — either operand being stack needs fixup
    if ((instr.op == X86Op::MovLoad || instr.op == X86Op::FloatLoad) &&
        instr.operand_count >= 2) {
        if (instr.operand(0).isStack() || instr.operand(1).isStack()) {
            return true;
        }
    }
    // MovStore/FloatStore: store to [dst] — either operand being stack needs fixup
    if ((instr.op == X86Op::MovStore || instr.op == X86Op::FloatStore) &&
        instr.operand_count >= 2) {
        if (instr.operand(0).isStack() || instr.operand(1).isStack()) {
            return true;
        }
    }
    // SSE: movsd/movss with stack dst or src needs fixup
    // (can't do movsd [mem], [rel label] or movsd [mem], [mem])
    if (isSSEOp(instr.op) && instr.operand_count >= 2) {
        if (instr.operand(0).isStack() || instr.operand(1).isStack()) {
            return true;
        }
    }
    return false;
}

void RegisterAllocator::rewrite(MachFunction& fn, const RegAllocation& alloc) {
    fn.stack_size = alloc.stack_size;
    for (uint32_t i = 0; i < NUM_CALLEE_SAVED; ++i) {
        fn.callee_saved_used[i] = alloc.callee_saved_used[i];
    }

    for (uint32_t b = 0; b < fn.block_count; ++b) {
        // First pass: rewrite operands
        for (uint32_t i = 0; i < fn.blocks[b].instr_count; ++i) {
            auto& instr = fn.blocks[b].instrs[i];
            for (uint8_t j = 0; j < instr.operand_count; ++j) {
                auto& op = instr.operand(j);
                op = rewriteOperand(op, alloc);
            }
        }

        // Second pass: fixup instructions with invalid stack operand combos
        // We build a new instruction list for the block
        std::vector<MachInstr> fixed;
        fixed.reserve(fn.blocks[b].instr_count * 2);  // worst case

        for (uint32_t i = 0; i < fn.blocks[b].instr_count; ++i) {
            auto& instr = fn.blocks[b].instrs[i];

            if (!needsFixup(instr)) {
                fixed.push_back(instr);
                continue;
            }

            // Use R11 as scratch (caller-saved, not used for args)
            PhysReg scratch = PhysReg::R11;

            if (instr.op == X86Op::Lea && instr.operand(0).isStack()) {
                // lea stack, src  →  lea r11, src; mov stack, r11
                auto stack_dst = instr.operand(0);
                instr.inline_ops[0] = MachOperand::precolored(scratch);
                fixed.push_back(instr);
                fixed.push_back(makeMov(stack_dst,
                                        MachOperand::precolored(scratch), 64));
            } else if ((instr.op == X86Op::Neg || instr.op == X86Op::Not) &&
                       instr.operand_count > 0 && instr.operand(0).isStack()) {
                // neg/not stack  →  mov r11, stack; neg/not r11; mov stack, r11
                auto stack_op = instr.operand(0);
                fixed.push_back(makeMov(MachOperand::precolored(scratch),
                                        stack_op, instr.width));
                instr.inline_ops[0] = MachOperand::precolored(scratch);
                fixed.push_back(instr);
                fixed.push_back(makeMov(stack_op,
                                        MachOperand::precolored(scratch),
                                        instr.width));
            } else if (instr.op == X86Op::Setcc && instr.operand_count > 0 &&
                       instr.operand(0).isStack()) {
                // setcc stack  →  setcc r11b; mov stack, r11
                auto stack_dst = instr.operand(0);
                instr.inline_ops[0] = MachOperand::precolored(scratch);
                fixed.push_back(instr);
                fixed.push_back(makeMov(stack_dst,
                                        MachOperand::precolored(scratch), 8));
            } else if (instr.op == X86Op::IDiv && instr.operand_count > 0 &&
                       instr.operand(0).isStack()) {
                // idiv stack  →  mov r11, stack; idiv r11
                auto stack_op = instr.operand(0);
                fixed.push_back(makeMov(MachOperand::precolored(scratch),
                                        stack_op, instr.width));
                instr.inline_ops[0] = MachOperand::precolored(scratch);
                fixed.push_back(instr);
            } else if (instr.op == X86Op::MovLoad && instr.operand_count >= 2) {
                bool dst_stack = instr.operand(0).isStack();
                bool src_stack = instr.operand(1).isStack();
                if (dst_stack && src_stack) {
                    // mov stack_dst, [stack_ptr]  →  mov r11, stack_ptr; mov r11, [r11]; mov stack_dst, r11
                    auto stack_dst = instr.operand(0);
                    auto stack_ptr = instr.operand(1);
                    fixed.push_back(makeMov(MachOperand::precolored(scratch),
                                            stack_ptr, 64));
                    MachInstr load(X86Op::MovLoad);
                    load.width = instr.width;
                    load.operand_count = 2;
                    load.inline_ops[0] = MachOperand::precolored(scratch);
                    load.inline_ops[1] = MachOperand::precolored(scratch);
                    fixed.push_back(load);
                    fixed.push_back(makeMov(stack_dst,
                                            MachOperand::precolored(scratch),
                                            instr.width));
                } else if (src_stack) {
                    // mov dst, [stack_ptr]  →  mov r11, stack_ptr; mov dst, [r11]
                    auto stack_ptr = instr.operand(1);
                    fixed.push_back(makeMov(MachOperand::precolored(scratch),
                                            stack_ptr, 64));
                    instr.inline_ops[1] = MachOperand::precolored(scratch);
                    fixed.push_back(instr);
                } else if (dst_stack) {
                    // mov stack_dst, [reg_ptr]  →  mov r11, [reg_ptr]; mov stack_dst, r11
                    auto stack_dst = instr.operand(0);
                    instr.inline_ops[0] = MachOperand::precolored(scratch);
                    fixed.push_back(instr);
                    fixed.push_back(makeMov(stack_dst,
                                            MachOperand::precolored(scratch),
                                            instr.width));
                }
            } else if (instr.op == X86Op::MovStore && instr.operand_count >= 2) {
                bool ptr_stack = instr.operand(0).isStack();
                bool val_stack = instr.operand(1).isStack();
                if (ptr_stack && val_stack) {
                    // mov [stack_ptr], stack_val  →  mov r11, stack_val; mov scratch2=r10, stack_ptr; mov [r10], r11
                    // Use both R11 (value) and R10 (pointer)
                    PhysReg scratch2 = PhysReg::R10;
                    auto stack_ptr = instr.operand(0);
                    auto stack_val = instr.operand(1);
                    fixed.push_back(makeMov(MachOperand::precolored(scratch),
                                            stack_val, instr.width));
                    fixed.push_back(makeMov(MachOperand::precolored(scratch2),
                                            stack_ptr, 64));
                    MachInstr store(X86Op::MovStore);
                    store.width = instr.width;
                    store.operand_count = 2;
                    store.inline_ops[0] = MachOperand::precolored(scratch2);
                    store.inline_ops[1] = MachOperand::precolored(scratch);
                    fixed.push_back(store);
                } else if (ptr_stack) {
                    // mov [stack_ptr], reg_val  →  mov r11, stack_ptr; mov [r11], reg_val
                    auto stack_ptr = instr.operand(0);
                    fixed.push_back(makeMov(MachOperand::precolored(scratch),
                                            stack_ptr, 64));
                    instr.inline_ops[0] = MachOperand::precolored(scratch);
                    fixed.push_back(instr);
                } else if (val_stack) {
                    // mov [reg_ptr], stack_val  →  mov r11, stack_val; mov [reg_ptr], r11
                    auto stack_val = instr.operand(1);
                    fixed.push_back(makeMov(MachOperand::precolored(scratch),
                                            stack_val, instr.width));
                    instr.inline_ops[1] = MachOperand::precolored(scratch);
                    fixed.push_back(instr);
                }
            } else if (instr.op == X86Op::FloatLoad && instr.operand_count >= 2) {
                // FloatLoad: movsd xmm_dst, [gpr_ptr]
                // If ptr is spilled: load ptr to r11 first
                bool dst_stack = instr.operand(0).isStack();
                bool ptr_stack = instr.operand(1).isStack();
                if (ptr_stack) {
                    auto stack_ptr = instr.operand(1);
                    fixed.push_back(makeMov(MachOperand::precolored(scratch),
                                            stack_ptr, 64));
                    instr.inline_ops[1] = MachOperand::precolored(scratch);
                }
                if (dst_stack) {
                    // Result goes to stack: use xmm15 scratch then store
                    PhysReg xmm_scratch = PhysReg::XMM15;
                    auto stack_dst = instr.operand(0);
                    instr.inline_ops[0] = MachOperand::precolored(xmm_scratch);
                    fixed.push_back(instr);
                    X86Op sse_op = (instr.width == 32) ? X86Op::Movss : X86Op::Movsd;
                    MachInstr store(sse_op);
                    store.width = instr.width;
                    store.operand_count = 2;
                    store.inline_ops[0] = stack_dst;
                    store.inline_ops[1] = MachOperand::precolored(xmm_scratch);
                    fixed.push_back(store);
                } else {
                    fixed.push_back(instr);
                }
            } else if (instr.op == X86Op::FloatStore && instr.operand_count >= 2) {
                // FloatStore: movsd [gpr_ptr], xmm_val
                // If ptr is spilled: load ptr to r11 first
                // If val is spilled: load to xmm15 first
                bool ptr_stack = instr.operand(0).isStack();
                bool val_stack = instr.operand(1).isStack();
                if (ptr_stack) {
                    auto stack_ptr = instr.operand(0);
                    fixed.push_back(makeMov(MachOperand::precolored(scratch),
                                            stack_ptr, 64));
                    instr.inline_ops[0] = MachOperand::precolored(scratch);
                }
                if (val_stack) {
                    PhysReg xmm_scratch = PhysReg::XMM15;
                    auto stack_val = instr.operand(1);
                    X86Op sse_op = (instr.width == 32) ? X86Op::Movss : X86Op::Movsd;
                    MachInstr load(sse_op);
                    load.width = instr.width;
                    load.operand_count = 2;
                    load.inline_ops[0] = MachOperand::precolored(xmm_scratch);
                    load.inline_ops[1] = stack_val;
                    fixed.push_back(load);
                    instr.inline_ops[1] = MachOperand::precolored(xmm_scratch);
                }
                fixed.push_back(instr);
            } else if (isSSEOp(instr.op) && instr.operand_count >= 2) {
                // SSE with stack operand(s)
                // Use XMM15 as scratch for SSE instructions
                PhysReg xmm_scratch = PhysReg::XMM15;
                bool dst_stack = instr.operand(0).isStack();
                bool src_stack = instr.operand(1).isStack();

                // Helper to make SSE mov (movsd for 64-bit, movss for 32-bit)
                auto makeSSEMov = [](MachOperand dst, MachOperand src, uint8_t w) {
                    X86Op op = (w == 32) ? X86Op::Movss : X86Op::Movsd;
                    MachInstr mi(op);
                    mi.width = w;
                    mi.operand_count = 2;
                    mi.inline_ops[0] = dst;
                    mi.inline_ops[1] = src;
                    return mi;
                };

                if (dst_stack && src_stack) {
                    // sse stack, stack → movsd xmm15, stack_src; sse xmm15, xmm15 (if not mov); movsd stack_dst, xmm15
                    auto stack_dst = instr.operand(0);
                    auto stack_src = instr.operand(1);
                    // For Movss/Movsd: just load src to scratch, store to dst
                    if (instr.op == X86Op::Movss || instr.op == X86Op::Movsd) {
                        fixed.push_back(makeSSEMov(
                            MachOperand::precolored(xmm_scratch), stack_src, instr.width));
                        fixed.push_back(makeSSEMov(
                            stack_dst, MachOperand::precolored(xmm_scratch), instr.width));
                    } else {
                        // ALU: load src to scratch, operate with stack_dst as dst
                        fixed.push_back(makeSSEMov(
                            MachOperand::precolored(xmm_scratch), stack_src, instr.width));
                        instr.inline_ops[1] = MachOperand::precolored(xmm_scratch);
                        // Still has stack_dst — ok for SSE alu (one mem operand)
                        fixed.push_back(instr);
                    }
                } else if (dst_stack) {
                    auto stack_dst = instr.operand(0);
                    if (instr.op == X86Op::Movss || instr.op == X86Op::Movsd) {
                        // movsd stack, src → movsd xmm15, src; movsd stack, xmm15
                        fixed.push_back(makeSSEMov(
                            MachOperand::precolored(xmm_scratch), instr.operand(1), instr.width));
                        fixed.push_back(makeSSEMov(
                            stack_dst, MachOperand::precolored(xmm_scratch), instr.width));
                    } else {
                        // alu stack, src → movsd xmm15, stack; alu xmm15, src; movsd stack, xmm15
                        fixed.push_back(makeSSEMov(
                            MachOperand::precolored(xmm_scratch), stack_dst, instr.width));
                        instr.inline_ops[0] = MachOperand::precolored(xmm_scratch);
                        fixed.push_back(instr);
                        fixed.push_back(makeSSEMov(
                            stack_dst, MachOperand::precolored(xmm_scratch), instr.width));
                    }
                } else if (src_stack) {
                    // sse reg, stack — this is actually valid for most SSE ops (one memory operand)
                    // But for safety, just emit as-is
                    fixed.push_back(instr);
                }
            } else if (instr.operand_count >= 2 &&
                       instr.operand(0).isStack() &&
                       instr.operand(1).isStack()) {
                // op stack, stack  →  mov r11, stack_src; op stack_dst, r11
                auto stack_src = instr.operand(1);
                fixed.push_back(makeMov(MachOperand::precolored(scratch),
                                        stack_src, instr.width));
                instr.inline_ops[1] = MachOperand::precolored(scratch);
                fixed.push_back(instr);
            } else {
                fixed.push_back(instr);
            }
        }

        // Replace block instructions with fixed ones
        if (fixed.size() != fn.blocks[b].instr_count) {
            auto* new_instrs = ctx_.arena.makeArray<MachInstr>(
                static_cast<uint32_t>(fixed.size()));
            for (uint32_t j = 0; j < fixed.size(); ++j) {
                new_instrs[j] = fixed[j];
            }
            fn.blocks[b].instrs = new_instrs;
            fn.blocks[b].instr_count = static_cast<uint32_t>(fixed.size());
        }
    }
}

// ============================================================================
// All-in-one: run
// ============================================================================

void RegisterAllocator::run(MachFunction& fn) {
    if (fn.is_intrinsic || fn.block_count == 0) return;

    auto intervals = computeIntervals(fn);
    auto alloc = allocate(intervals, fn.struct_alloc_bytes);
    rewrite(fn, alloc);
}

} // namespace kern
