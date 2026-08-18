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

    std::string emit(const char* source, OutputFormat fmt = OutputFormat::Macho64) {
        auto [mach, lir] = buildAll(source, fmt);
        std::ostringstream oss;
        NASMEmitter emitter(oss, fmt);
        emitter.emitModule(*mach, *lir);
        return oss.str();
    }

    std::pair<MachModule*, LIRModule*> buildAll(const char* source,
                                                 OutputFormat fmt) {
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

        InstructionSelector isel(ctx, fmt);
        MachModule* mach = isel.select(*lir);

        RegisterAllocator ra(ctx);
        for (uint32_t i = 0; i < mach->fn_count; ++i) {
            ra.run(mach->functions[i]);
        }

        return {mach, lir};
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

// ============================================================================
// Parallel Move Cycle-Breaking Tests
// ============================================================================

// Helper to build a ParallelMove MachInstr from (dst, src) register pairs
static MachInstr makeTestParallelMove(std::initializer_list<std::pair<PhysReg, PhysReg>> pairs,
                                       MachOperand* storage) {
    MachInstr instr(X86Op::Pseudo_ParallelMove);
    uint8_t count = 0;
    for (auto& [dst, src] : pairs) {
        storage[count++] = MachOperand::precolored(dst);
        storage[count++] = MachOperand::precolored(src);
    }
    instr.operand_count = count;
    if (count <= MACH_INLINE_OPERANDS) {
        for (uint8_t i = 0; i < count; ++i) {
            instr.inline_ops[i] = storage[i];
        }
    } else {
        instr.heap_ops = storage;
    }
    return instr;
}

TEST_F(EmitterTest, ParallelMoveNoCycle) {
    // rdi ← rax, rsi ← rbx  — no cycle, should produce sequential movs
    MachOperand ops[4];
    auto instr = makeTestParallelMove({{PhysReg::RDI, PhysReg::RAX},
                                    {PhysReg::RSI, PhysReg::RBX}}, ops);

    std::ostringstream oss;
    NASMEmitter emitter(oss);
    MachInstr instrs[2];
    instrs[0] = instr;
    instrs[1] = makeRet();
    MachBlock block;
    block.label = ".entry";
    block.instrs = instrs;
    block.instr_count = 2;
    MachFunction fn;
    fn.name = "pm_nocycle";
    fn.blocks = &block;
    fn.block_count = 1;
    fn.stack_size = 0;
    for (auto& cs : fn.callee_saved_used) cs = false;

    emitter.emitFunction(fn);

    std::string out = oss.str();
    // Should have two mov instructions, no xchg
    EXPECT_NE(out.find("mov rdi, rax"), std::string::npos);
    EXPECT_NE(out.find("mov rsi, rbx"), std::string::npos);
    EXPECT_EQ(out.find("xchg"), std::string::npos);
}

TEST_F(EmitterTest, ParallelMoveTwoRegCycle) {
    // rax ← rbx, rbx ← rax  — 2-cycle, should use xchg
    MachOperand ops[4];
    auto instr = makeTestParallelMove({{PhysReg::RAX, PhysReg::RBX},
                                    {PhysReg::RBX, PhysReg::RAX}}, ops);

    std::ostringstream oss;
    NASMEmitter emitter(oss);
    MachInstr instrs[2];
    instrs[0] = instr;
    instrs[1] = makeRet();
    MachBlock block;
    block.label = ".entry";
    block.instrs = instrs;
    block.instr_count = 2;
    MachFunction fn;
    fn.name = "pm_swap";
    fn.blocks = &block;
    fn.block_count = 1;
    fn.stack_size = 0;
    for (auto& cs : fn.callee_saved_used) cs = false;

    emitter.emitFunction(fn);

    std::string out = oss.str();
    EXPECT_NE(out.find("xchg"), std::string::npos);
}

TEST_F(EmitterTest, ParallelMoveThreeRegCycle) {
    // rax ← rbx, rbx ← rcx, rcx ← rax  — 3-cycle, should use r11 scratch
    MachOperand ops[6];
    auto instr = makeTestParallelMove({{PhysReg::RAX, PhysReg::RBX},
                                    {PhysReg::RBX, PhysReg::RCX},
                                    {PhysReg::RCX, PhysReg::RAX}}, ops);

    std::ostringstream oss;
    NASMEmitter emitter(oss);
    MachInstr instrs[2];
    instrs[0] = instr;
    instrs[1] = makeRet();
    MachBlock block;
    block.label = ".entry";
    block.instrs = instrs;
    block.instr_count = 2;
    MachFunction fn;
    fn.name = "pm_triple";
    fn.blocks = &block;
    fn.block_count = 1;
    fn.stack_size = 0;
    for (auto& cs : fn.callee_saved_used) cs = false;

    emitter.emitFunction(fn);

    std::string out = oss.str();
    // Should use r11 as scratch for the 3-cycle
    EXPECT_NE(out.find("r11"), std::string::npos);
    // Should NOT have xchg (3-cycles use scratch, not xchg)
    EXPECT_EQ(out.find("xchg"), std::string::npos);
}

TEST_F(EmitterTest, ParallelMoveMixed) {
    // Mix of non-cyclic + 2-cycle:
    // rdi ← rsi (non-cyclic part) + rax ← rbx, rbx ← rax (cycle)
    MachOperand ops[6];
    auto instr = makeTestParallelMove({{PhysReg::RDI, PhysReg::RSI},
                                    {PhysReg::RAX, PhysReg::RBX},
                                    {PhysReg::RBX, PhysReg::RAX}}, ops);

    std::ostringstream oss;
    NASMEmitter emitter(oss);
    MachInstr instrs[2];
    instrs[0] = instr;
    instrs[1] = makeRet();
    MachBlock block;
    block.label = ".entry";
    block.instrs = instrs;
    block.instr_count = 2;
    MachFunction fn;
    fn.name = "pm_mixed";
    fn.blocks = &block;
    fn.block_count = 1;
    fn.stack_size = 0;
    for (auto& cs : fn.callee_saved_used) cs = false;

    emitter.emitFunction(fn);

    std::string out = oss.str();
    // Non-cyclic move should be a plain mov
    EXPECT_NE(out.find("mov rdi, rsi"), std::string::npos);
    // Cycle should be resolved with xchg
    EXPECT_NE(out.find("xchg"), std::string::npos);
}

// ============================================================================
// ELF Output Format Tests
// ============================================================================

TEST_F(EmitterTest, ElfNoUnderscorePrefix) {
    auto asm_out = emit("fn main() -> i64 { 42 }", OutputFormat::Elf64);
    // ELF: no underscore prefix on symbols
    EXPECT_NE(asm_out.find("main:"), std::string::npos);
    EXPECT_EQ(asm_out.find("_main:"), std::string::npos);
}

TEST_F(EmitterTest, ElfStartWrapper) {
    auto asm_out = emit("fn main() -> i64 { 0 }", OutputFormat::Elf64);
    // ELF: start wrapper uses Linux syscall 60
    EXPECT_NE(asm_out.find("global start"), std::string::npos);
    EXPECT_NE(asm_out.find("start:"), std::string::npos);
    EXPECT_NE(asm_out.find("call main"), std::string::npos);
    EXPECT_NE(asm_out.find("mov  rax, 60"), std::string::npos);
    // Should NOT have macOS syscall
    EXPECT_EQ(asm_out.find("0x02000001"), std::string::npos);
}

TEST_F(EmitterTest, ElfFunctionCall) {
    auto asm_out = emit(
        "fn foo(x: i64) -> i64 { x }\n"
        "fn main() -> i64 { val r: i64 = foo(42)\n r }",
        OutputFormat::Elf64);
    EXPECT_NE(asm_out.find("foo:"), std::string::npos);
    EXPECT_NE(asm_out.find("call foo"), std::string::npos);
    EXPECT_EQ(asm_out.find("_foo"), std::string::npos);
}

TEST_F(EmitterTest, MachoHasUnderscorePrefix) {
    auto asm_out = emit("fn main() -> i64 { 42 }", OutputFormat::Macho64);
    // Mach-O: underscore prefix on all symbols
    EXPECT_NE(asm_out.find("_main:"), std::string::npos);
    EXPECT_NE(asm_out.find("global _start"), std::string::npos);
    EXPECT_NE(asm_out.find("_start:"), std::string::npos);
    EXPECT_NE(asm_out.find("call _main"), std::string::npos);
    EXPECT_NE(asm_out.find("0x02000001"), std::string::npos);
}

// ============================================================================
// Interrupt Handler Tests
// ============================================================================

TEST_F(EmitterTest, InterruptSavesXMM) {
    auto asm_out = emit(
        "@interrupt\n"
        "fn timer_handler() -> Unit { asm { \"nop\" } }\n"
        "fn main() -> i64 { 42 }");
    // Regular @interrupt should save XMM registers
    EXPECT_NE(asm_out.find("sub rsp, 256"), std::string::npos);
    EXPECT_NE(asm_out.find("movdqu [rsp+0], xmm0"), std::string::npos);
    EXPECT_NE(asm_out.find("movdqu xmm0, [rsp+0]"), std::string::npos);
    EXPECT_NE(asm_out.find("add rsp, 256"), std::string::npos);
    EXPECT_NE(asm_out.find("iretq"), std::string::npos);
}

TEST_F(EmitterTest, InterruptNofpSkipsXMM) {
    auto asm_out = emit(
        "@interrupt_nofp\n"
        "fn timer_handler() -> Unit { asm { \"nop\" } }\n"
        "fn main() -> i64 { 42 }");
    // @interrupt_nofp should NOT save/restore XMM registers
    // But should still have push/pop GPRs and iretq
    EXPECT_NE(asm_out.find("push rax"), std::string::npos);
    EXPECT_NE(asm_out.find("push r15"), std::string::npos);
    EXPECT_NE(asm_out.find("iretq"), std::string::npos);
    // Must NOT have XMM save/restore
    EXPECT_EQ(asm_out.find("sub rsp, 256"), std::string::npos);
    EXPECT_EQ(asm_out.find("movdqu"), std::string::npos);
    EXPECT_EQ(asm_out.find("add rsp, 256"), std::string::npos);
}

// ============================================================================
// Section flags emission
// ============================================================================

TEST_F(EmitterTest, SectionFlagsELF) {
    auto asm_out = emit(
        "@section(\".kern_init\", \"axp\") fn f() -> i64 { 42 }\n"
        "fn main() -> i64 { f() }",
        OutputFormat::Elf64);
    // Should contain section with NASM flags
    EXPECT_NE(asm_out.find("section .kern_init alloc exec progbits"), std::string::npos);
}

TEST_F(EmitterTest, SectionFlagsNobits) {
    auto asm_out = emit(
        "@section(\".kern_bss\", \"awn\") fn f() -> i64 { 42 }\n"
        "fn main() -> i64 { f() }",
        OutputFormat::Elf64);
    EXPECT_NE(asm_out.find("section .kern_bss alloc write nobits"), std::string::npos);
}

// ============================================================================
// @no_red_zone propagation
// ============================================================================

TEST_F(EmitterTest, NoRedZonePropagation) {
    auto [mach, lir] = buildAll(
        "@no_red_zone fn f() -> i64 { 42 }\n"
        "fn main() -> i64 { f() }");
    // Verify flag propagated to MachFunction
    bool found = false;
    for (uint32_t i = 0; i < mach->fn_count; ++i) {
        if (mach->functions[i].name == "f") {
            EXPECT_TRUE(mach->functions[i].is_no_red_zone);
            found = true;
        }
    }
    EXPECT_TRUE(found);
}

// ============================================================================
// Global variable visibility
// ============================================================================

TEST_F(EmitterTest, NoRedZoneEnforcement) {
    auto [mach, lir] = buildAll(
        "@no_red_zone fn f() -> i64 { 42 }\n"
        "fn main() -> i64 { f() }");
    // Verify flag propagated AND find f's stack_size
    bool found = false;
    for (uint32_t i = 0; i < mach->fn_count; ++i) {
        if (mach->functions[i].name == "f") {
            EXPECT_TRUE(mach->functions[i].is_no_red_zone);
            found = true;
        }
    }
    EXPECT_TRUE(found);
    // Emit and verify the no_red_zone function allocates at least 128 bytes
    std::ostringstream oss;
    NASMEmitter emitter(oss, OutputFormat::Macho64);
    emitter.emitModule(*mach, *lir);
    auto asm_out = oss.str();
    // Must have sub rsp for the no_red_zone function
    EXPECT_NE(asm_out.find("sub rsp, 128"), std::string::npos) << asm_out;
}

// ============================================================================
// Global variable visibility
// ============================================================================

TEST_F(EmitterTest, GlobalWeakVisibility) {
    auto asm_out = emit(
        "@weak pub static val X: i64 = 42\n"
        "fn main() -> i64 { X }",
        OutputFormat::Elf64);
    EXPECT_NE(asm_out.find("weak"), std::string::npos);
}

TEST_F(EmitterTest, GlobalHiddenVisibility) {
    auto asm_out = emit(
        "@hidden pub static val X: i64 = 42\n"
        "fn main() -> i64 { X }",
        OutputFormat::Elf64);
    EXPECT_NE(asm_out.find("hidden"), std::string::npos);
}
