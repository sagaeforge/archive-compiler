#include "kern/backend/RegisterAllocator.h"
#include "kern/backend/InstructionSelector.h"
#include "kern/backend/MachIR.h"
#include "kern/hir/HIRBuilder.h"
#include "kern/hir/HIRPasses.h"
#include "kern/lir/LIRBuilder.h"
#include "kern/lexer/Lexer.h"
#include "kern/parser/Parser.h"
#include "kern/support/CompilationContext.h"

#include <gtest/gtest.h>

using namespace kern;

class RegisterAllocatorTest : public ::testing::Test {
protected:
    CompilationContext ctx;

    Module* parse(const char* source) {
        ctx.diag.setSource(source);
        Lexer lexer(source, "test.kern", ctx.diag);
        Parser parser(lexer, ctx.arena, ctx.diag);
        return parser.parseModule();
    }

    MachModule* buildMachIR(const char* source) {
        auto* ast = parse(source);
        EXPECT_FALSE(ctx.diag.hasErrors());

        HIRBuilder hir_builder(ctx);
        HIRModule* hir = hir_builder.build(ast);
        EXPECT_FALSE(ctx.diag.hasErrors());

        HIRPassManager pm;
        pm.add<PurityAnalysisPass>();
        pm.add<TailCallAnalysisPass>();
        pm.run(*hir, ctx);

        LIRBuilder lir_builder(ctx);
        LIRModule* lir = lir_builder.build(hir);

        InstructionSelector isel(ctx);
        return isel.select(*lir);
    }

    // Run regalloc on all functions in module
    void runRegAlloc(MachModule* mod) {
        RegisterAllocator ra(ctx);
        for (uint32_t i = 0; i < mod->fn_count; ++i) {
            ra.run(mod->functions[i]);
        }
    }

    // Check if any vreg remains (all should be physical after regalloc)
    bool hasVirtualRegs(const MachFunction& fn) {
        for (uint32_t b = 0; b < fn.block_count; ++b) {
            for (uint32_t i = 0; i < fn.blocks[b].instr_count; ++i) {
                const auto& instr = fn.blocks[b].instrs[i];
                for (uint8_t j = 0; j < instr.operand_count; ++j) {
                    if (instr.operand(j).isVirtual()) return true;
                }
            }
        }
        return false;
    }

    bool hasOp(const MachFunction& fn, X86Op op) {
        for (uint32_t b = 0; b < fn.block_count; ++b) {
            for (uint32_t i = 0; i < fn.blocks[b].instr_count; ++i) {
                if (fn.blocks[b].instrs[i].op == op) return true;
            }
        }
        return false;
    }
};

// ============================================================================
// Basic Allocation Tests
// ============================================================================

TEST_F(RegisterAllocatorTest, SimpleConst) {
    auto* mod = buildMachIR("fn main() -> i64 { 42 }");
    runRegAlloc(mod);
    auto& fn = mod->functions[0];
    EXPECT_FALSE(hasVirtualRegs(fn));
}

TEST_F(RegisterAllocatorTest, IntAdd) {
    auto* mod = buildMachIR("fn add(a: i64, b: i64) -> i64 { a + b }");
    runRegAlloc(mod);
    auto& fn = mod->functions[0];
    EXPECT_FALSE(hasVirtualRegs(fn));
}

TEST_F(RegisterAllocatorTest, IfExpr) {
    auto* mod = buildMachIR(
        "fn f(x: i64) -> i64 { if x > 0 { 1 } else { 0 } }");
    runRegAlloc(mod);
    auto& fn = mod->functions[0];
    EXPECT_FALSE(hasVirtualRegs(fn));
}

TEST_F(RegisterAllocatorTest, MultipleFunctions) {
    auto* mod = buildMachIR(
        "fn add(a: i64, b: i64) -> i64 { a + b }\n"
        "fn main() -> i64 { val r: i64 = add(20, 22)\n r }");
    runRegAlloc(mod);
    for (uint32_t i = 0; i < mod->fn_count; ++i) {
        if (!mod->functions[i].is_intrinsic) {
            EXPECT_FALSE(hasVirtualRegs(mod->functions[i]))
                << "Function " << mod->functions[i].name << " has virtual regs";
        }
    }
}

TEST_F(RegisterAllocatorTest, IntrinsicSkipped) {
    auto* mod = buildMachIR(
        "fn print(x: i64) -> i64 = intrinsic\n"
        "fn main() -> i64 { 0 }");
    runRegAlloc(mod);
    EXPECT_TRUE(mod->functions[0].is_intrinsic);
    EXPECT_EQ(mod->functions[0].block_count, 0u);
}

// ============================================================================
// Liveness Tests
// ============================================================================

TEST_F(RegisterAllocatorTest, ComputeIntervals) {
    auto* mod = buildMachIR("fn add(a: i64, b: i64) -> i64 { a + b }");
    auto& fn = mod->functions[0];

    RegisterAllocator ra(ctx);
    auto intervals = ra.computeIntervals(fn);
    EXPECT_GT(intervals.size(), 0u);

    // Check that intervals are sorted by start
    for (size_t i = 1; i < intervals.size(); ++i) {
        EXPECT_LE(intervals[i-1].start, intervals[i].start);
    }
}

TEST_F(RegisterAllocatorTest, FixedIntervalsPreserved) {
    auto* mod = buildMachIR("fn main() -> i64 { 42 }");
    auto& fn = mod->functions[0];

    RegisterAllocator ra(ctx);
    auto intervals = ra.computeIntervals(fn);

    // Should have at least one fixed interval (rax from ret)
    bool found_fixed = false;
    for (const auto& li : intervals) {
        if (li.is_fixed) {
            found_fixed = true;
            EXPECT_NE(li.hint, PhysReg::NONE);
        }
    }
    EXPECT_TRUE(found_fixed);
}

// ============================================================================
// Stack Alignment Tests
// ============================================================================

TEST_F(RegisterAllocatorTest, StackAligned) {
    auto* mod = buildMachIR("fn main() -> i64 { 42 }");
    runRegAlloc(mod);
    auto& fn = mod->functions[0];
    // Stack size should be 0 or multiple of 8
    EXPECT_EQ(fn.stack_size % 8, 0u);
}

// ============================================================================
// Spill Tests (many vregs)
// ============================================================================

TEST_F(RegisterAllocatorTest, VarBindingAllocated) {
    auto* mod = buildMachIR(
        "fn f() -> i64 {\n"
        "  var x: i64 = 10\n"
        "  x = 20\n"
        "  x\n"
        "}");
    runRegAlloc(mod);
    auto& fn = mod->functions[0];
    EXPECT_FALSE(hasVirtualRegs(fn));
}

TEST_F(RegisterAllocatorTest, MatchExprAllocated) {
    auto* mod = buildMachIR(
        "fn f(x: i64) -> i64 {\n"
        "  match x {\n"
        "    0 => 10,\n"
        "    1 => 20,\n"
        "    _ => 30\n"
        "  }\n"
        "}");
    runRegAlloc(mod);
    auto& fn = mod->functions[0];
    EXPECT_FALSE(hasVirtualRegs(fn));
}

TEST_F(RegisterAllocatorTest, DivisionAllocated) {
    auto* mod = buildMachIR("fn d(a: i64, b: i64) -> i64 { a / b }");
    runRegAlloc(mod);
    auto& fn = mod->functions[0];
    EXPECT_FALSE(hasVirtualRegs(fn));
}

TEST_F(RegisterAllocatorTest, NestedIfAllocated) {
    auto* mod = buildMachIR(
        "fn f(x: i64) -> i64 {\n"
        "  if x > 0 {\n"
        "    if x > 10 { 100 } else { 50 }\n"
        "  } else { 0 }\n"
        "}");
    runRegAlloc(mod);
    auto& fn = mod->functions[0];
    EXPECT_FALSE(hasVirtualRegs(fn));
}

TEST_F(RegisterAllocatorTest, NegUnaryAllocated) {
    auto* mod = buildMachIR("fn neg(a: i64) -> i64 { -a }");
    runRegAlloc(mod);
    auto& fn = mod->functions[0];
    EXPECT_FALSE(hasVirtualRegs(fn));
}

TEST_F(RegisterAllocatorTest, ComparisonAllocated) {
    auto* mod = buildMachIR("fn lt(a: i64, b: i64) -> bool { a < b }");
    runRegAlloc(mod);
    auto& fn = mod->functions[0];
    EXPECT_FALSE(hasVirtualRegs(fn));
}
