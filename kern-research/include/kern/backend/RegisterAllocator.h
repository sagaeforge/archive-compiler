#pragma once
#include "kern/backend/MachIR.h"
#include "kern/support/CompilationContext.h"
#include <unordered_map>
#include <vector>

namespace kern {

// ============================================================================
// Live Interval
// ============================================================================

struct LiveInterval {
    uint32_t vreg;
    uint32_t start;     // first instruction index (global numbering)
    uint32_t end;        // last use instruction index
    PhysReg hint = PhysReg::NONE;    // preferred physical register (ABI, etc.)
    bool is_fixed = false;           // pre-colored, cannot be moved
    bool is_float = false;           // needs XMM register
};

// ============================================================================
// Allocation Result
// ============================================================================

struct RegAllocation {
    std::unordered_map<uint32_t, PhysReg> reg_map;    // vreg → phys reg
    std::unordered_map<uint32_t, int32_t> spill_map;  // vreg → stack offset
    uint32_t stack_size = 0;                            // total frame size
    bool callee_saved_used[NUM_CALLEE_SAVED] = {};
};

// ============================================================================
// Register Allocator (Linear Scan)
// ============================================================================

class RegisterAllocator {
    [[maybe_unused]] CompilationContext& ctx_;

public:
    explicit RegisterAllocator(CompilationContext& ctx) : ctx_(ctx) {}

    // Phase 1: compute live intervals from MachFunction
    std::vector<LiveInterval> computeIntervals(const MachFunction& fn);

    // Phase 2: linear scan allocation
    RegAllocation allocate(std::vector<LiveInterval>& intervals,
                          uint32_t struct_alloc_bytes = 0);

    // Phase 3: rewrite function to use physical registers + insert prologue/epilogue
    void rewrite(MachFunction& fn, const RegAllocation& alloc);

    // All-in-one: compute intervals, allocate, rewrite
    void run(MachFunction& fn);

private:
    // Track which physical registers are occupied at a given point
    struct RegState {
        bool gpr_used[NUM_ALLOCATABLE_GPRS] = {};
        bool xmm_used[NUM_ALLOCATABLE_XMMS] = {};
    };

    // Find a free register for an interval
    PhysReg findFreeGPR(const RegState& state, PhysReg hint = PhysReg::NONE);
    PhysReg findFreeXMM(const RegState& state, PhysReg hint = PhysReg::NONE);

    // Mark a callee-saved register in the allocation
    void markCalleeSaved(PhysReg reg, RegAllocation& alloc);

    // Global instruction numbering
    struct InstrIndex {
        uint32_t block_idx;
        uint32_t instr_idx;
        uint32_t global_idx;
    };

    // Build global instruction numbering for a function
    std::vector<InstrIndex> buildNumbering(const MachFunction& fn);
};

} // namespace kern
