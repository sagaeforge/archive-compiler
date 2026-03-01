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

static void scanOperandDefs(const MachInstr& instr, uint32_t idx,
                             std::unordered_map<uint32_t, LiveInterval>& intervals) {
    // For most instructions, operand[0] is the destination (def)
    if (instr.operand_count > 0) {
        const auto& op = instr.operand(0);
        if (op.isVirtual()) {
            recordVRegUse(intervals, op.vreg, idx, false);
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
        instr.op == X86Op::Ucomisd || instr.op == X86Op::Ucomiss) {
        start = 0;
    }
    // Setcc: operand[0] is dst (def)
    // Ret: no explicit operands
    // IDiv: operand[0] is src (divisor) — special case
    if (instr.op == X86Op::IDiv || instr.op == X86Op::Neg ||
        instr.op == X86Op::Not) {
        start = 0;  // all operands are use+def for destructive ops
    }

    for (uint8_t i = start; i < instr.operand_count; ++i) {
        const auto& op = instr.operand(i);
        if (op.isVirtual()) {
            recordVRegUse(intervals, op.vreg, idx, false);
        } else if (op.isPhysical()) {
            recordPhysUse(intervals, op.phys, idx);
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
                recordVRegUse(intervals, op.vreg, idx, false);
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
}

std::vector<LiveInterval>
RegisterAllocator::computeIntervals(const MachFunction& fn) {
    std::unordered_map<uint32_t, LiveInterval> interval_map;

    uint32_t global_idx = 0;
    for (uint32_t b = 0; b < fn.block_count; ++b) {
        for (uint32_t i = 0; i < fn.blocks[b].instr_count; ++i) {
            const auto& instr = fn.blocks[b].instrs[i];
            scanOperandDefs(instr, global_idx, interval_map);
            scanOperandUses(instr, global_idx, interval_map);
            global_idx++;
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
RegisterAllocator::allocate(std::vector<LiveInterval>& intervals) {
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
    int32_t next_spill_offset = -8;  // grows downward from rbp

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

void RegisterAllocator::rewrite(MachFunction& fn, const RegAllocation& alloc) {
    fn.stack_size = alloc.stack_size;
    for (uint32_t i = 0; i < NUM_CALLEE_SAVED; ++i) {
        fn.callee_saved_used[i] = alloc.callee_saved_used[i];
    }

    for (uint32_t b = 0; b < fn.block_count; ++b) {
        for (uint32_t i = 0; i < fn.blocks[b].instr_count; ++i) {
            auto& instr = fn.blocks[b].instrs[i];
            for (uint8_t j = 0; j < instr.operand_count; ++j) {
                auto& op = instr.operand(j);
                op = rewriteOperand(op, alloc);
            }
        }
    }
}

// ============================================================================
// All-in-one: run
// ============================================================================

void RegisterAllocator::run(MachFunction& fn) {
    if (fn.is_intrinsic || fn.block_count == 0) return;

    auto intervals = computeIntervals(fn);
    auto alloc = allocate(intervals);
    rewrite(fn, alloc);
}

} // namespace kern
