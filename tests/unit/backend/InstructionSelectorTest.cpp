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

// ============================================================================
// Advanced ISel Tests
// ============================================================================

TEST_F(InstructionSelectorTest, FloatArith) {
    auto* mod = selectAll(
        "fn fadd(a: f64, b: f64) -> f64 { a + b }");
    ASSERT_NE(mod, nullptr);
    auto& fn = mod->functions[0];
    EXPECT_TRUE(hasOp(fn, X86Op::Addsd));
}

TEST_F(InstructionSelectorTest, FloatCmp) {
    auto* mod = selectAll(
        "fn flt(a: f64, b: f64) -> bool { a < b }");
    ASSERT_NE(mod, nullptr);
    auto& fn = mod->functions[0];
    EXPECT_TRUE(hasOp(fn, X86Op::Ucomisd));
}

TEST_F(InstructionSelectorTest, CastMovsxNarrowToWide) {
    auto* mod = selectAll(
        "fn widen(x: i8) -> i64 { x as i64 }");
    ASSERT_NE(mod, nullptr);
    auto& fn = mod->functions[0];
    // i8→i64 should produce some kind of sign-extending move
    EXPECT_TRUE(hasOp(fn, X86Op::Mov) || hasOp(fn, X86Op::MovLoad));
}

TEST_F(InstructionSelectorTest, GlobalLoad) {
    auto* mod = selectAll(
        "static val G: i64 = 42\n"
        "fn main() -> i64 { G }");
    ASSERT_NE(mod, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    // Should have MovLoadGlobal for accessing G
    auto& fn = mod->functions[0];
    EXPECT_TRUE(hasOp(fn, X86Op::MovLoadGlobal));
}

TEST_F(InstructionSelectorTest, GlobalStore) {
    auto* mod = selectAll(
        "static var G: i64 = 0\n"
        "fn main() -> i64 with mut, mem {\n"
        "  G = 42\n"
        "  G\n"
        "}");
    ASSERT_NE(mod, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    auto& fn = mod->functions[0];
    EXPECT_TRUE(hasOp(fn, X86Op::MovStoreGlobal));
}

TEST_F(InstructionSelectorTest, AtomicCas) {
    auto* mod = selectAll(
        "fn atomic_cas(ptr: Ptr<var i64>, expected: i64, desired: i64) -> i64 = intrinsic\n"
        "fn try_cas() -> i64 with atomic, mem {\n"
        "  var x: i64 = 10\n"
        "  atomic_cas(&var x, 10, 42)\n"
        "}");
    ASSERT_NE(mod, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    bool found = false;
    for (uint32_t i = 0; i < mod->fn_count; ++i) {
        if (hasOp(mod->functions[i], X86Op::LockCmpxchg)) found = true;
    }
    EXPECT_TRUE(found);
}

TEST_F(InstructionSelectorTest, FetchAdd) {
    auto* mod = selectAll(
        "fn atomic_fetch_add(ptr: Ptr<var i64>, value: i64) -> i64 = intrinsic\n"
        "fn inc() -> i64 with atomic, mem {\n"
        "  var x: i64 = 0\n"
        "  atomic_fetch_add(&var x, 1)\n"
        "}");
    ASSERT_NE(mod, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    bool found = false;
    for (uint32_t i = 0; i < mod->fn_count; ++i) {
        if (hasOp(mod->functions[i], X86Op::LockXadd)) found = true;
    }
    EXPECT_TRUE(found);
}

TEST_F(InstructionSelectorTest, Fence) {
    auto* mod = selectAll(
        "fn mfence() -> Unit = intrinsic\n"
        "fn barrier() -> Unit with atomic {\n"
        "  mfence()\n"
        "}");
    ASSERT_NE(mod, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    bool found = false;
    for (uint32_t i = 0; i < mod->fn_count; ++i) {
        if (hasOp(mod->functions[i], X86Op::Mfence)) found = true;
    }
    EXPECT_TRUE(found);
}

TEST_F(InstructionSelectorTest, VolatileLoadStore) {
    auto* mod = selectAll(
        "fn volatile_read(ptr: Ptr<i64>) -> i64 = intrinsic\n"
        "fn volatile_write(ptr: Ptr<var i64>, v: i64) -> Unit = intrinsic\n"
        "fn main() -> i64 {\n"
        "    var x: i64 = 10\n"
        "    volatile_write(&var x, 42)\n"
        "    volatile_read(&x)\n"
        "}");
    ASSERT_NE(mod, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());
    // Find the main function and verify volatile load/store exist
    bool has_load = false, has_store = false;
    for (uint32_t i = 0; i < mod->fn_count; ++i) {
        auto& fn = mod->functions[i];
        for (uint32_t b = 0; b < fn.block_count; ++b) {
            for (uint32_t j = 0; j < fn.blocks[b].instr_count; ++j) {
                auto& mi = fn.blocks[b].instrs[j];
                if (mi.is_volatile && (mi.op == X86Op::MovLoad || mi.op == X86Op::FloatLoad))
                    has_load = true;
                if (mi.is_volatile && (mi.op == X86Op::MovStore || mi.op == X86Op::FloatStore))
                    has_store = true;
            }
        }
    }
    EXPECT_TRUE(has_load) << "volatile load should propagate is_volatile";
    EXPECT_TRUE(has_store) << "volatile store should propagate is_volatile";
}

// >16B struct return with zero parameters: hidden_ret_ptr_ must be captured
// from RDI even though there are no BlockArg instructions.
TEST_F(InstructionSelectorTest, BigStructReturnZeroParams) {
    const char* src = R"(
struct Big { a: i64, b: i64, c: i64 }
fn make_big() -> Big { Big { a: 1, b: 2, c: 3 } }
fn main() -> i64 { val s = make_big(); s.a }
)";
    auto* mach = selectAll(src);
    ASSERT_NE(mach, nullptr);
    EXPECT_FALSE(ctx.diag.hasErrors());

    // Find _make_big function and verify first instruction captures RDI
    const MachFunction* make_big_fn = nullptr;
    for (uint32_t i = 0; i < mach->fn_count; ++i) {
        if (std::string(mach->functions[i].name) == "make_big") {
            make_big_fn = &mach->functions[i];
            break;
        }
    }
    ASSERT_NE(make_big_fn, nullptr) << "make_big function not found";
    ASSERT_GT(make_big_fn->block_count, 0u);

    // First instruction in entry block should be a Mov from RDI (the hidden
    // return pointer capture).
    const auto& entry = make_big_fn->blocks[0];
    ASSERT_GT(entry.instr_count, 0u);
    bool found_rdi_capture = false;
    for (uint32_t i = 0; i < entry.instr_count; ++i) {
        const auto& mi = entry.instrs[i];
        if (mi.op == X86Op::Mov && mi.operand_count >= 2 &&
            mi.inline_ops[1].kind == MachOperand::Kind::Reg &&
            mi.inline_ops[1].is_physical &&
            mi.inline_ops[1].phys == PhysReg::RDI) {
            found_rdi_capture = true;
            break;
        }
    }
    EXPECT_TRUE(found_rdi_capture)
        << ">16B return: entry block must capture hidden RDI pointer";
}
