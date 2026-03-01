#include "kern/backend/InstructionSelector.h"
#include "kern/backend/MachIR.h"
#include "kern/backend/MachIRDump.h"
#include "kern/hir/HIRBuilder.h"
#include "kern/hir/HIRPasses.h"
#include "kern/lir/LIRBuilder.h"
#include "kern/lir/LIR.h"
#include "kern/lexer/Lexer.h"
#include "kern/parser/Parser.h"
#include "kern/support/CompilationContext.h"

#include <gtest/gtest.h>
#include <sstream>
#include <string>

using namespace kern;

class InstructionSelectorTest : public ::testing::Test {
protected:
    CompilationContext ctx;

    Module* parse(const char* source) {
        ctx.diag.setSource(source);
        Lexer lexer(source, "test.kern", ctx.diag);
        Parser parser(lexer, ctx.arena, ctx.diag);
        return parser.parseModule();
    }

    LIRModule* buildLIR(const char* source) {
        auto* ast = parse(source);
        EXPECT_FALSE(ctx.diag.hasErrors()) << "Parse errors";

        HIRBuilder hir_builder(ctx);
        HIRModule* hir = hir_builder.build(ast);
        EXPECT_FALSE(ctx.diag.hasErrors()) << "HIR errors";

        HIRPassManager pm;
        pm.add<PurityAnalysisPass>();
        pm.add<TailCallAnalysisPass>();
        pm.run(*hir, ctx);

        LIRBuilder lir_builder(ctx);
        return lir_builder.build(hir);
    }

    MachModule* selectAll(const char* source) {
        auto* lir = buildLIR(source);
        EXPECT_NE(lir, nullptr);
        InstructionSelector isel(ctx);
        return isel.select(*lir);
    }

    // Helper: dump MachIR to string
    std::string dumpToString(const MachModule* mod) {
        std::ostringstream oss;
        for (uint32_t i = 0; i < mod->fn_count; ++i) {
            dumpMachFunction(mod->functions[i], ctx.types, oss);
        }
        return oss.str();
    }

    // Helper: find instruction in function
    bool hasOp(const MachFunction& fn, X86Op op) {
        for (uint32_t b = 0; b < fn.block_count; ++b) {
            for (uint32_t i = 0; i < fn.blocks[b].instr_count; ++i) {
                if (fn.blocks[b].instrs[i].op == op) return true;
            }
        }
        return false;
    }

    // Helper: count instructions of a type
    uint32_t countOp(const MachFunction& fn, X86Op op) {
        uint32_t c = 0;
        for (uint32_t b = 0; b < fn.block_count; ++b) {
            for (uint32_t i = 0; i < fn.blocks[b].instr_count; ++i) {
                if (fn.blocks[b].instrs[i].op == op) c++;
            }
        }
        return c;
    }
};

// ============================================================================
// Basic Tests
// ============================================================================

TEST_F(InstructionSelectorTest, ConstInt) {
    auto* mod = selectAll("fn main() -> i64 { 42 }");
    ASSERT_NE(mod, nullptr);
    EXPECT_EQ(mod->fn_count, 1u);
    auto& fn = mod->functions[0];
    EXPECT_GT(fn.block_count, 0u);
    EXPECT_TRUE(hasOp(fn, X86Op::Mov));
    EXPECT_TRUE(hasOp(fn, X86Op::Ret));
}

TEST_F(InstructionSelectorTest, ConstBool) {
    auto* mod = selectAll("fn main() -> bool { true }");
    ASSERT_NE(mod, nullptr);
    auto& fn = mod->functions[0];
    EXPECT_TRUE(hasOp(fn, X86Op::Mov));
}

TEST_F(InstructionSelectorTest, IntAdd) {
    auto* mod = selectAll("fn add(a: i64, b: i64) -> i64 { a + b }");
    ASSERT_NE(mod, nullptr);
    auto& fn = mod->functions[0];
    EXPECT_TRUE(hasOp(fn, X86Op::Add));
}

TEST_F(InstructionSelectorTest, IntSub) {
    auto* mod = selectAll("fn sub(a: i64, b: i64) -> i64 { a - b }");
    ASSERT_NE(mod, nullptr);
    auto& fn = mod->functions[0];
    EXPECT_TRUE(hasOp(fn, X86Op::Sub));
}

TEST_F(InstructionSelectorTest, IntMul) {
    auto* mod = selectAll("fn mul(a: i64, b: i64) -> i64 { a * b }");
    ASSERT_NE(mod, nullptr);
    auto& fn = mod->functions[0];
    EXPECT_TRUE(hasOp(fn, X86Op::IMul));
}

TEST_F(InstructionSelectorTest, IntDiv) {
    auto* mod = selectAll("fn div(a: i64, b: i64) -> i64 { a / b }");
    ASSERT_NE(mod, nullptr);
    auto& fn = mod->functions[0];
    EXPECT_TRUE(hasOp(fn, X86Op::IDiv));
    EXPECT_TRUE(hasOp(fn, X86Op::Cqo));
}

TEST_F(InstructionSelectorTest, IntDivCqoPattern) {
    // Division should produce: mov rax, lhs; cqo; idiv rhs; mov dst, rax
    auto* mod = selectAll("fn d(a: i64, b: i64) -> i64 { a / b }");
    ASSERT_NE(mod, nullptr);
    auto& fn = mod->functions[0];
    EXPECT_GE(countOp(fn, X86Op::Mov), 2u);  // mov rax + mov result
    EXPECT_EQ(countOp(fn, X86Op::Cqo), 1u);
    EXPECT_EQ(countOp(fn, X86Op::IDiv), 1u);
}

TEST_F(InstructionSelectorTest, IntCmpLt) {
    auto* mod = selectAll("fn lt(a: i64, b: i64) -> bool { a < b }");
    ASSERT_NE(mod, nullptr);
    auto& fn = mod->functions[0];
    EXPECT_TRUE(hasOp(fn, X86Op::Cmp));
    EXPECT_TRUE(hasOp(fn, X86Op::Setcc));
    // Check xor before cmp for zeroing
    EXPECT_TRUE(hasOp(fn, X86Op::Xor));
}

TEST_F(InstructionSelectorTest, UnaryNeg) {
    auto* mod = selectAll("fn neg(a: i64) -> i64 { -a }");
    ASSERT_NE(mod, nullptr);
    auto& fn = mod->functions[0];
    EXPECT_TRUE(hasOp(fn, X86Op::Neg));
}

TEST_F(InstructionSelectorTest, UnaryNot) {
    auto* mod = selectAll("fn inv(a: bool) -> bool { not a }");
    ASSERT_NE(mod, nullptr);
    auto& fn = mod->functions[0];
    EXPECT_TRUE(hasOp(fn, X86Op::Test));
    EXPECT_TRUE(hasOp(fn, X86Op::Setcc));
}

TEST_F(InstructionSelectorTest, IfExpr) {
    auto* mod = selectAll("fn f(x: i64) -> i64 { if x > 0 { 1 } else { 0 } }");
    ASSERT_NE(mod, nullptr);
    auto& fn = mod->functions[0];
    EXPECT_TRUE(hasOp(fn, X86Op::Jcc));
    EXPECT_TRUE(hasOp(fn, X86Op::Jmp));
}

TEST_F(InstructionSelectorTest, FunctionCall) {
    // double(21) in tail position → tail call (jmp)
    // val x = double(21) prevents tail call, so we use that
    auto* mod = selectAll(
        "fn double(x: i64) -> i64 { x + x }\n"
        "fn main() -> i64 { val r: i64 = double(21)\n r }");
    ASSERT_NE(mod, nullptr);
    EXPECT_EQ(mod->fn_count, 2u);
    auto& main_fn = mod->functions[1];
    EXPECT_TRUE(hasOp(main_fn, X86Op::Call));
}

TEST_F(InstructionSelectorTest, TailCall) {
    // double(21) in tail position → jmp (not call)
    auto* mod = selectAll(
        "fn double(x: i64) -> i64 { x + x }\n"
        "fn main() -> i64 { double(21) }");
    ASSERT_NE(mod, nullptr);
    auto& main_fn = mod->functions[1];
    // Tail call emits jmp, not call
    EXPECT_TRUE(hasOp(main_fn, X86Op::Jmp));
}

TEST_F(InstructionSelectorTest, ValBinding) {
    auto* mod = selectAll("fn f() -> i64 { val x: i64 = 10\n x + 5 }");
    ASSERT_NE(mod, nullptr);
    auto& fn = mod->functions[0];
    EXPECT_TRUE(hasOp(fn, X86Op::Mov));
    EXPECT_TRUE(hasOp(fn, X86Op::Add));
}

TEST_F(InstructionSelectorTest, VarBinding) {
    auto* mod = selectAll(
        "fn f() -> i64 {\n"
        "  var x: i64 = 10\n"
        "  x = 20\n"
        "  x\n"
        "}");
    ASSERT_NE(mod, nullptr);
    auto& fn = mod->functions[0];
    // Should have struct_alloc → lea (stack slot)
    EXPECT_TRUE(hasOp(fn, X86Op::Lea));
}

TEST_F(InstructionSelectorTest, IntrinsicSkipped) {
    auto* mod = selectAll(
        "fn print(x: i64) -> i64 = intrinsic\n"
        "fn main() -> i64 { 0 }");
    ASSERT_NE(mod, nullptr);
    EXPECT_EQ(mod->fn_count, 2u);
    auto& print_fn = mod->functions[0];
    EXPECT_TRUE(print_fn.is_intrinsic);
    EXPECT_EQ(print_fn.block_count, 0u);
}

TEST_F(InstructionSelectorTest, RetMovsToRAX) {
    auto* mod = selectAll("fn main() -> i64 { 42 }");
    auto& fn = mod->functions[0];

    // Last block should end with mov rax, vreg; ret
    auto& last_block = fn.blocks[fn.block_count - 1];
    bool found_ret = false;
    bool found_rax_mov = false;
    for (uint32_t i = 0; i < last_block.instr_count; ++i) {
        auto& mi = last_block.instrs[i];
        if (mi.op == X86Op::Ret) found_ret = true;
        if (mi.op == X86Op::Mov && mi.dst().isPhysical() &&
            mi.dst().phys == PhysReg::RAX) {
            found_rax_mov = true;
        }
    }
    EXPECT_TRUE(found_ret);
    EXPECT_TRUE(found_rax_mov);
}

TEST_F(InstructionSelectorTest, MultipleFunctions) {
    auto* mod = selectAll(
        "fn add(a: i64, b: i64) -> i64 { a + b }\n"
        "fn main() -> i64 { add(20, 22) }");
    EXPECT_EQ(mod->fn_count, 2u);
    EXPECT_EQ(mod->functions[0].name, "add");
    EXPECT_EQ(mod->functions[1].name, "main");
}

TEST_F(InstructionSelectorTest, DumpOutput) {
    auto* mod = selectAll("fn main() -> i64 { 42 }");
    std::string out = dumpToString(mod);
    EXPECT_NE(out.find("fn @main"), std::string::npos);
    EXPECT_NE(out.find("mov"), std::string::npos);
    EXPECT_NE(out.find("ret"), std::string::npos);
}

TEST_F(InstructionSelectorTest, DivUsesRAX) {
    auto* mod = selectAll("fn d(a: i64, b: i64) -> i64 { a / b }");
    auto& fn = mod->functions[0];

    // Should have mov to rax (for dividend)
    bool found_rax = false;
    for (uint32_t b = 0; b < fn.block_count; ++b) {
        for (uint32_t i = 0; i < fn.blocks[b].instr_count; ++i) {
            auto& mi = fn.blocks[b].instrs[i];
            if (mi.op == X86Op::Mov && mi.dst().isPhysical() &&
                mi.dst().phys == PhysReg::RAX) {
                found_rax = true;
            }
        }
    }
    EXPECT_TRUE(found_rax);
}

TEST_F(InstructionSelectorTest, StructAllocEmitsLea) {
    auto* mod = selectAll(
        "fn f() -> i64 {\n"
        "  var x: i64 = 10\n"
        "  x\n"
        "}");
    auto& fn = mod->functions[0];
    // StructAlloc → lea (stack slot, no inline sub rsp)
    EXPECT_TRUE(hasOp(fn, X86Op::Lea));
    EXPECT_GT(fn.struct_alloc_bytes, 0u);
}

TEST_F(InstructionSelectorTest, MatchExpr) {
    auto* mod = selectAll(
        "fn f(x: i64) -> i64 {\n"
        "  match x {\n"
        "    0 => 10,\n"
        "    _ => 20\n"
        "  }\n"
        "}");
    ASSERT_NE(mod, nullptr);
    auto& fn = mod->functions[0];
    // Match generates comparisons and branches
    EXPECT_TRUE(hasOp(fn, X86Op::Cmp));
    EXPECT_TRUE(hasOp(fn, X86Op::Jcc));
}

TEST_F(InstructionSelectorTest, LogicalAndOr) {
    auto* mod = selectAll(
        "fn f(a: bool, b: bool) -> bool { a and b }");
    ASSERT_NE(mod, nullptr);
    auto& fn = mod->functions[0];
    // Short-circuit generates branches
    EXPECT_TRUE(hasOp(fn, X86Op::Test));
}

TEST_F(InstructionSelectorTest, LargeStructReturnHiddenPtr) {
    // Closure with 3 captures → 32B struct → >16B → hidden pointer ABI
    auto* mod = selectAll(
        "fn make(a: i64, b: i64, c: i64) -> fn(i64) -> i64 {\n"
        "    { x: i64 => x + a + b + c }\n"
        "}\n"
        "fn main() -> i64 {\n"
        "    val f: fn(i64) -> i64 = make(1, 2, 3)\n"
        "    f(10)\n"
        "}\n");
    ASSERT_NE(mod, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    // make() should have MovStore instructions (writing to hidden ptr buffer)
    // and main() should have a call to _make
    bool found_make = false;
    for (uint32_t i = 0; i < mod->fn_count; ++i) {
        if (mod->functions[i].name.find("make") != std::string_view::npos) {
            found_make = true;
            // >16B ret: must have MovStore to write through hidden ptr
            EXPECT_TRUE(hasOp(mod->functions[i], X86Op::MovStore));
        }
    }
    EXPECT_TRUE(found_make);
}

TEST_F(InstructionSelectorTest, SmallStructReturnPacked) {
    // 16B struct return (2 fields) → packed into RAX+RDX
    auto* mod = selectAll(
        "struct Pair { x: i64, y: i64 }\n"
        "fn make_pair(a: i64, b: i64) -> Pair {\n"
        "    Pair { x: a, y: b }\n"
        "}\n"
        "fn main() -> i64 {\n"
        "    val p: Pair = make_pair(10, 32)\n"
        "    p.x + p.y\n"
        "}\n");
    ASSERT_NE(mod, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    // make_pair should pack into RAX via MovLoad (reading from struct base)
    bool found_make_pair = false;
    for (uint32_t i = 0; i < mod->fn_count; ++i) {
        if (mod->functions[i].name.find("make_pair") != std::string_view::npos) {
            found_make_pair = true;
            EXPECT_TRUE(hasOp(mod->functions[i], X86Op::MovLoad));
        }
    }
    EXPECT_TRUE(found_make_pair);
}
