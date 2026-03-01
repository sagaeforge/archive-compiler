#include "kern/backend/Emitter.h"
#include "kern/backend/InstructionSelector.h"
#include "kern/backend/RegisterAllocator.h"
#include "kern/backend/MachIR.h"
#include "kern/hir/HIRBuilder.h"
#include "kern/hir/HIRPasses.h"
#include "kern/lir/LIRBuilder.h"
#include "kern/lir/LIR.h"
#include "kern/lexer/Lexer.h"
#include "kern/parser/Parser.h"
#include "kern/support/CompilationContext.h"

#include <gtest/gtest.h>
#include <sstream>

using namespace kern;

class EmitterTest : public ::testing::Test {
protected:
    CompilationContext ctx;

    Module* parse(const char* source) {
        ctx.diag.setSource(source);
        Lexer lexer(source, "test.kern", ctx.diag);
        Parser parser(lexer, ctx.arena, ctx.diag);
        return parser.parseModule();
    }

    // Full pipeline: source → LIR + MachIR + RegAlloc
    std::pair<MachModule*, LIRModule*> buildAll(const char* source) {
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
        MachModule* mach = isel.select(*lir);

        RegisterAllocator ra(ctx);
        for (uint32_t i = 0; i < mach->fn_count; ++i) {
            ra.run(mach->functions[i]);
        }

        return {mach, lir};
    }

    std::string emit(const char* source) {
        auto [mach, lir] = buildAll(source);
        std::ostringstream oss;
        NASMEmitter emitter(oss);
        emitter.emitModule(*mach, *lir);
        return oss.str();
    }
};

// ============================================================================
// Basic Emission Tests
// ============================================================================

TEST_F(EmitterTest, SimpleConst) {
    auto asm_out = emit("fn main() -> i64 { 42 }");
    EXPECT_NE(asm_out.find("section .text"), std::string::npos);
    EXPECT_NE(asm_out.find("_main:"), std::string::npos);
    EXPECT_NE(asm_out.find("push rbp"), std::string::npos);
    EXPECT_NE(asm_out.find("mov rbp, rsp"), std::string::npos);
    EXPECT_NE(asm_out.find("42"), std::string::npos);
    EXPECT_NE(asm_out.find("ret"), std::string::npos);
}

TEST_F(EmitterTest, StartWrapper) {
    auto asm_out = emit("fn main() -> i64 { 0 }");
    EXPECT_NE(asm_out.find("global _start"), std::string::npos);
    EXPECT_NE(asm_out.find("_start:"), std::string::npos);
    EXPECT_NE(asm_out.find("call _main"), std::string::npos);
    EXPECT_NE(asm_out.find("syscall"), std::string::npos);
}

TEST_F(EmitterTest, NoStartWithoutMain) {
    auto asm_out = emit("fn foo() -> i64 { 42 }");
    EXPECT_EQ(asm_out.find("_start:"), std::string::npos);
}

TEST_F(EmitterTest, IntAdd) {
    auto asm_out = emit("fn add(a: i64, b: i64) -> i64 { a + b }");
    EXPECT_NE(asm_out.find("add"), std::string::npos);
}

TEST_F(EmitterTest, FunctionLabel) {
    auto asm_out = emit(
        "fn foo(x: i64) -> i64 { x }\n"
        "fn main() -> i64 { val r: i64 = foo(42)\n r }");
    EXPECT_NE(asm_out.find("_foo:"), std::string::npos);
    EXPECT_NE(asm_out.find("_main:"), std::string::npos);
    EXPECT_NE(asm_out.find("call _foo"), std::string::npos);
}

TEST_F(EmitterTest, Prologue) {
    auto asm_out = emit("fn main() -> i64 { 42 }");
    // Must have push rbp and mov rbp, rsp
    auto push_pos = asm_out.find("push rbp");
    auto mov_pos = asm_out.find("mov rbp, rsp");
    EXPECT_NE(push_pos, std::string::npos);
    EXPECT_NE(mov_pos, std::string::npos);
    EXPECT_LT(push_pos, mov_pos);
}

TEST_F(EmitterTest, Epilogue) {
    auto asm_out = emit("fn main() -> i64 { 42 }");
    // Must have pop rbp and ret
    EXPECT_NE(asm_out.find("pop rbp"), std::string::npos);
    EXPECT_NE(asm_out.find("ret"), std::string::npos);
}

TEST_F(EmitterTest, IntrinsicNotEmitted) {
    auto asm_out = emit(
        "fn print(x: i64) -> i64 = intrinsic\n"
        "fn main() -> i64 { 0 }");
    // print should not have a label
    EXPECT_EQ(asm_out.find("_print:"), std::string::npos);
}

TEST_F(EmitterTest, ComparisonSetcc) {
    auto asm_out = emit(
        "fn lt(a: i64, b: i64) -> bool { a < b }");
    EXPECT_NE(asm_out.find("cmp"), std::string::npos);
    EXPECT_NE(asm_out.find("setl"), std::string::npos);
}

TEST_F(EmitterTest, Division) {
    auto asm_out = emit(
        "fn d(a: i64, b: i64) -> i64 { a / b }");
    EXPECT_NE(asm_out.find("cqo"), std::string::npos);
    EXPECT_NE(asm_out.find("idiv"), std::string::npos);
}

TEST_F(EmitterTest, Negation) {
    auto asm_out = emit("fn neg(a: i64) -> i64 { -a }");
    EXPECT_NE(asm_out.find("neg"), std::string::npos);
}

TEST_F(EmitterTest, ConditionalJump) {
    auto asm_out = emit(
        "fn f(x: i64) -> i64 { if x > 0 { 1 } else { 0 } }");
    // Should have conditional and unconditional jumps
    EXPECT_NE(asm_out.find("jne"), std::string::npos);
    EXPECT_NE(asm_out.find("jmp"), std::string::npos);
}

// ============================================================================
// Instruction-level Tests
// ============================================================================

TEST_F(EmitterTest, StackFrameEmitted) {
    auto asm_out = emit(
        "fn f() -> i64 {\n"
        "  var x: i64 = 10\n"
        "  x = 20\n"
        "  x\n"
        "}");
    // Stack frame: should have sub rsp for local storage
    // (may or may not have sub rsp depending on allocation)
    EXPECT_NE(asm_out.find("_f:"), std::string::npos);
    EXPECT_NE(asm_out.find("push rbp"), std::string::npos);
}

TEST_F(EmitterTest, EmitSingleFunction) {
    MachInstr instrs[2];
    instrs[0] = makeMov(MachOperand::precolored(PhysReg::RAX),
                        MachOperand::immediate(42));
    instrs[1] = makeRet();

    MachBlock block;
    block.label = ".entry";
    block.instrs = instrs;
    block.instr_count = 2;

    MachFunction fn;
    fn.name = "test";
    fn.blocks = &block;
    fn.block_count = 1;
    fn.stack_size = 0;
    fn.is_intrinsic = false;
    for (auto& cs : fn.callee_saved_used) cs = false;

    std::ostringstream oss;
    NASMEmitter emitter(oss);
    emitter.emitFunction(fn);

    std::string out = oss.str();
    EXPECT_NE(out.find("_test:"), std::string::npos);
    EXPECT_NE(out.find("push rbp"), std::string::npos);
    EXPECT_NE(out.find("mov rax, 42"), std::string::npos);
    EXPECT_NE(out.find("pop rbp"), std::string::npos);
    EXPECT_NE(out.find("ret"), std::string::npos);
}

TEST_F(EmitterTest, EmitRodataFloat) {
    GlobalData g;
    g.kind = GlobalData::FloatConst;
    g.index = 0;
    g.label = "_fc0";
    g.float_const = {3.14, false};

    std::ostringstream oss;
    NASMEmitter emitter(oss);
    emitter.emitRodata(&g, 1);

    std::string out = oss.str();
    EXPECT_NE(out.find("section .rodata"), std::string::npos);
    EXPECT_NE(out.find("_fc0:"), std::string::npos);
    EXPECT_NE(out.find("dq"), std::string::npos);
}

TEST_F(EmitterTest, EmitRodataString) {
    const char* data = "hello";
    GlobalData g;
    g.kind = GlobalData::StringLit;
    g.index = 0;
    g.label = "_str0";
    g.string_lit = {data, 5};

    std::ostringstream oss;
    NASMEmitter emitter(oss);
    emitter.emitRodata(&g, 1);

    std::string out = oss.str();
    EXPECT_NE(out.find("section .rodata"), std::string::npos);
    EXPECT_NE(out.find("_str0:"), std::string::npos);
    EXPECT_NE(out.find("db"), std::string::npos);
}
