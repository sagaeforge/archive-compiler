#include "kern/parser/Parser.h"
#include "kern/lexer/Lexer.h"
#include "kern/ir/IRBuilder.h"
#include "kern/ir/KernIR.h"
#include "kern/sema/TypeChecker.h"
#include "kern/sema/PurityChecker.h"
#include "kern/support/Arena.h"
#include "kern/support/Diagnostic.h"
#include <gtest/gtest.h>
#include <sstream>

using namespace kern;

struct IRResult {
    std::string source;
    Arena arena;
    DiagnosticEngine diag;
    IRModule ir;
};

static IRResult buildIR(std::string src) {
    IRResult r;
    r.source = std::move(src);
    Lexer lexer(r.source, "test.kern", r.diag);
    Parser parser(lexer, r.arena, r.diag);
    Module* mod = parser.parseModule();
    EXPECT_FALSE(r.diag.hasErrors());

    TypeChecker tc(r.diag);
    tc.check(mod);

    IRBuilder builder;
    r.ir = builder.build(mod, tc);
    return r;
}

static bool hasOpcode(const IRFunction& fn, IROpcode op) {
    for (const auto& block : fn.blocks) {
        for (const auto& instr : block.instrs) {
            if (instr.op == op) return true;
        }
    }
    return false;
}

// ===== Existing tests =====

TEST(IRTest, SimpleConstant) {
    auto r = buildIR("fn main() -> i64 { 42 }");
    ASSERT_EQ(r.ir.functions.size(), 1u);
    EXPECT_EQ(r.ir.functions[0].name, "main");
    EXPECT_FALSE(r.ir.functions[0].blocks.empty());
}

TEST(IRTest, FibonacciIR) {
    auto r = buildIR(
        "fn fib(n: i64) -> i64 {\n"
        "    if n <= 1 { n }\n"
        "    else { fib(n - 1) + fib(n - 2) }\n"
        "}"
    );
    ASSERT_EQ(r.ir.functions.size(), 1u);
    EXPECT_EQ(r.ir.functions[0].name, "fib");
    EXPECT_GE(r.ir.functions[0].blocks.size(), 3u);
}

TEST(IRTest, IRDump) {
    auto r = buildIR("fn main() -> i64 { 42 }");
    std::ostringstream out;
    dumpIR(r.ir, out);
    std::string dump = out.str();
    EXPECT_NE(dump.find("fn main"), std::string::npos);
    EXPECT_NE(dump.find("const_int"), std::string::npos);
    EXPECT_NE(dump.find("ret"), std::string::npos);
}

TEST(IRTest, FunctionCall) {
    auto r = buildIR(
        "fn double_val(x: i64) -> i64 { x + x }\n"
        "fn main() -> i64 { double_val(21) }"
    );
    ASSERT_EQ(r.ir.functions.size(), 2u);

    bool found_call = false;
    for (const auto& block : r.ir.functions[1].blocks) {
        for (const auto& instr : block.instrs) {
            if (instr.op == IROpcode::Call) {
                found_call = true;
                EXPECT_EQ(instr.callee_name, "double_val");
            }
        }
    }
    EXPECT_TRUE(found_call);
}

// ===== New TDD tests =====

// --- Val binding generates ConstInt + reference ---
TEST(IRTest, ValBinding) {
    auto r = buildIR(
        "fn main() -> i64 {\n"
        "    val x: i64 = 10\n"
        "    x\n"
        "}"
    );
    ASSERT_EQ(r.ir.functions.size(), 1u);
    EXPECT_TRUE(hasOpcode(r.ir.functions[0], IROpcode::ConstInt));
    EXPECT_TRUE(hasOpcode(r.ir.functions[0], IROpcode::Ret));
}

// --- Arithmetic generates Add ---
TEST(IRTest, ArithmeticAdd) {
    auto r = buildIR("fn main() -> i64 { 1 + 2 }");
    ASSERT_EQ(r.ir.functions.size(), 1u);
    EXPECT_TRUE(hasOpcode(r.ir.functions[0], IROpcode::Add));
}

// --- Subtraction ---
TEST(IRTest, ArithmeticSub) {
    auto r = buildIR("fn main() -> i64 { 10 - 3 }");
    ASSERT_EQ(r.ir.functions.size(), 1u);
    EXPECT_TRUE(hasOpcode(r.ir.functions[0], IROpcode::Sub));
}

// --- Multiplication ---
TEST(IRTest, ArithmeticMul) {
    auto r = buildIR("fn main() -> i64 { 6 * 7 }");
    ASSERT_EQ(r.ir.functions.size(), 1u);
    EXPECT_TRUE(hasOpcode(r.ir.functions[0], IROpcode::Mul));
}

// --- Division ---
TEST(IRTest, ArithmeticDiv) {
    auto r = buildIR("fn main() -> i64 { 84 / 2 }");
    ASSERT_EQ(r.ir.functions.size(), 1u);
    EXPECT_TRUE(hasOpcode(r.ir.functions[0], IROpcode::Div));
}

// --- Comparison generates ICmpLe ---
TEST(IRTest, Comparison) {
    auto r = buildIR("fn main() -> bool { 1 <= 2 }");
    ASSERT_EQ(r.ir.functions.size(), 1u);
    EXPECT_TRUE(hasOpcode(r.ir.functions[0], IROpcode::ICmpLe));
}

// --- Negation emits Neg opcode ---
TEST(IRTest, UnaryNeg) {
    auto r = buildIR("fn main() -> i64 { -42 }");
    ASSERT_EQ(r.ir.functions.size(), 1u);
    EXPECT_TRUE(hasOpcode(r.ir.functions[0], IROpcode::Neg));
}

// --- Not emits Not opcode ---
TEST(IRTest, UnaryNot) {
    auto r = buildIR("fn main() -> bool { not true }");
    ASSERT_EQ(r.ir.functions.size(), 1u);
    EXPECT_TRUE(hasOpcode(r.ir.functions[0], IROpcode::Not));
}

// --- If/else generates CondBranch ---
TEST(IRTest, IfElseGeneratesCondBranch) {
    auto r = buildIR(
        "fn main() -> i64 {\n"
        "    if true { 1 } else { 0 }\n"
        "}"
    );
    ASSERT_EQ(r.ir.functions.size(), 1u);
    EXPECT_TRUE(hasOpcode(r.ir.functions[0], IROpcode::CondBranch));
}

// --- Nested if generates multiple CondBranch ---
TEST(IRTest, NestedIf) {
    auto r = buildIR(
        "fn main() -> i64 {\n"
        "    if true { if false { 1 } else { 2 } } else { 3 }\n"
        "}"
    );
    ASSERT_EQ(r.ir.functions.size(), 1u);
    int cond_count = 0;
    for (const auto& block : r.ir.functions[0].blocks) {
        for (const auto& instr : block.instrs) {
            if (instr.op == IROpcode::CondBranch) cond_count++;
        }
    }
    EXPECT_GE(cond_count, 2);
}

// --- Multiple functions in IR ---
TEST(IRTest, MultipleFunctions) {
    auto r = buildIR(
        "fn a() -> i64 { 1 }\n"
        "fn b() -> i64 { 2 }\n"
        "fn main() -> i64 { a() + b() }"
    );
    ASSERT_EQ(r.ir.functions.size(), 3u);
    EXPECT_EQ(r.ir.functions[0].name, "a");
    EXPECT_EQ(r.ir.functions[1].name, "b");
    EXPECT_EQ(r.ir.functions[2].name, "main");
}

// --- Parameters become values ---
TEST(IRTest, Parameters) {
    auto r = buildIR("fn add(a: i64, b: i64) -> i64 { a + b }");
    ASSERT_EQ(r.ir.functions.size(), 1u);
    EXPECT_EQ(r.ir.functions[0].param_values.size(), 2u);
    EXPECT_EQ(r.ir.functions[0].param_names.size(), 2u);
    EXPECT_EQ(r.ir.functions[0].param_names[0], "a");
    EXPECT_EQ(r.ir.functions[0].param_names[1], "b");
}

// --- IR dump contains all expected elements for fib ---
TEST(IRTest, FibDumpComprehensive) {
    auto r = buildIR(
        "fn fib(n: i64) -> i64 {\n"
        "    if n <= 1 { n } else { fib(n - 1) + fib(n - 2) }\n"
        "}"
    );
    std::ostringstream out;
    dumpIR(r.ir, out);
    std::string dump = out.str();
    EXPECT_NE(dump.find("fn fib"), std::string::npos);
    EXPECT_NE(dump.find("icmp_le"), std::string::npos);
    EXPECT_NE(dump.find("condbr"), std::string::npos);
    EXPECT_NE(dump.find("call"), std::string::npos);
    EXPECT_NE(dump.find("add"), std::string::npos);
    EXPECT_NE(dump.find("sub"), std::string::npos);
}

// ===== Coverage improvement tests =====

// --- Logical And uses short-circuit (CondBranch) ---
TEST(IRTest, LogicalAnd) {
    auto r = buildIR("fn main() -> bool { true and true }");
    ASSERT_EQ(r.ir.functions.size(), 1u);
    EXPECT_TRUE(hasOpcode(r.ir.functions[0], IROpcode::CondBranch));
}

// --- Logical Or uses short-circuit (CondBranch) ---
TEST(IRTest, LogicalOr) {
    auto r = buildIR("fn main() -> bool { false or true }");
    ASSERT_EQ(r.ir.functions.size(), 1u);
    EXPECT_TRUE(hasOpcode(r.ir.functions[0], IROpcode::CondBranch));
}

// --- ICmpEq ---
TEST(IRTest, ICmpEq) {
    auto r = buildIR("fn main() -> bool { 1 == 2 }");
    EXPECT_TRUE(hasOpcode(r.ir.functions[0], IROpcode::ICmpEq));
}

// --- ICmpNe ---
TEST(IRTest, ICmpNe) {
    auto r = buildIR("fn main() -> bool { 1 != 2 }");
    EXPECT_TRUE(hasOpcode(r.ir.functions[0], IROpcode::ICmpNe));
}

// --- ICmpLt ---
TEST(IRTest, ICmpLt) {
    auto r = buildIR("fn main() -> bool { 1 < 2 }");
    EXPECT_TRUE(hasOpcode(r.ir.functions[0], IROpcode::ICmpLt));
}

// --- ICmpGt and ICmpGe ---
TEST(IRTest, ICmpGt) {
    auto r = buildIR("fn main() -> bool { 1 > 2 }");
    EXPECT_TRUE(hasOpcode(r.ir.functions[0], IROpcode::ICmpGt));
}

TEST(IRTest, ICmpGe) {
    auto r = buildIR("fn main() -> bool { 1 >= 2 }");
    EXPECT_TRUE(hasOpcode(r.ir.functions[0], IROpcode::ICmpGe));
}

// --- If without else (unit default) ---
TEST(IRTest, IfWithoutElse) {
    auto r = buildIR("fn main() -> i64 { if true { 42 } }");
    ASSERT_EQ(r.ir.functions.size(), 1u);
    EXPECT_TRUE(hasOpcode(r.ir.functions[0], IROpcode::CondBranch));
}

// --- Block without result ---
TEST(IRTest, BlockWithoutResult) {
    auto r = buildIR(
        "fn main() -> i64 {\n"
        "    val x: i64 = 10\n"
        "}"
    );
    ASSERT_EQ(r.ir.functions.size(), 1u);
    EXPECT_TRUE(hasOpcode(r.ir.functions[0], IROpcode::ConstInt));
}

// --- VarDecl in IR ---
TEST(IRTest, VarDecl) {
    auto r = buildIR(
        "fn main() -> i64 {\n"
        "    var x: i64 = 10\n"
        "    x\n"
        "}"
    );
    ASSERT_EQ(r.ir.functions.size(), 1u);
    EXPECT_TRUE(hasOpcode(r.ir.functions[0], IROpcode::ConstInt));
}

// --- IR dump includes unconditional branch ---
TEST(IRTest, IRDumpBranch) {
    auto r = buildIR(
        "fn main() -> i64 { if true { 1 } else { 0 } }"
    );
    std::ostringstream out;
    dumpIR(r.ir, out);
    std::string dump = out.str();
    EXPECT_NE(dump.find("br "), std::string::npos);
}

// ===== M2.2: IR type tests =====

// --- Constant i64 has IRType::I64 ---
TEST(IRTest, ConstIntTypeI64) {
    auto r = buildIR("fn main() -> i64 { 42 }");
    auto& fn = r.ir.functions[0];
    bool found = false;
    for (auto& block : fn.blocks) {
        for (auto& instr : block.instrs) {
            if (instr.op == IROpcode::ConstInt && instr.imm_value == 42) {
                EXPECT_EQ(instr.type, IRType::I64);
                found = true;
            }
        }
    }
    EXPECT_TRUE(found);
}

// --- BoolLit has IRType::Bool ---
TEST(IRTest, BoolLitTypeBool) {
    auto r = buildIR("fn main() -> bool { true }");
    auto& fn = r.ir.functions[0];
    bool found = false;
    for (auto& block : fn.blocks) {
        for (auto& instr : block.instrs) {
            if (instr.op == IROpcode::ConstInt && instr.imm_value == 1) {
                EXPECT_EQ(instr.type, IRType::Bool);
                found = true;
            }
        }
    }
    EXPECT_TRUE(found);
}

// --- Parameter types propagated ---
TEST(IRTest, ParamTypesI64) {
    auto r = buildIR("fn add(a: i64, b: i64) -> i64 { a + b }");
    auto& fn = r.ir.functions[0];
    ASSERT_EQ(fn.param_types.size(), 2u);
    EXPECT_EQ(fn.param_types[0], IRType::I64);
    EXPECT_EQ(fn.param_types[1], IRType::I64);
}

// --- i32 parameter types ---
TEST(IRTest, ParamTypesI32) {
    auto r = buildIR("fn add32(a: i32, b: i32) -> i32 { a + b }");
    auto& fn = r.ir.functions[0];
    ASSERT_EQ(fn.param_types.size(), 2u);
    EXPECT_EQ(fn.param_types[0], IRType::I32);
    EXPECT_EQ(fn.param_types[1], IRType::I32);
}

// --- Comparison result has IRType::Bool ---
TEST(IRTest, ComparisonTypeBool) {
    auto r = buildIR("fn main() -> bool { 1 < 2 }");
    auto& fn = r.ir.functions[0];
    bool found = false;
    for (auto& block : fn.blocks) {
        for (auto& instr : block.instrs) {
            if (instr.op == IROpcode::ICmpLt) {
                EXPECT_EQ(instr.type, IRType::Bool);
                found = true;
            }
        }
    }
    EXPECT_TRUE(found);
}

// --- FunctionMeta exists with default purity ---
TEST(IRTest, FunctionMetaDefault) {
    auto r = buildIR("fn main() -> i64 { 42 }");
    auto& fn = r.ir.functions[0];
    EXPECT_EQ(fn.meta.purity, Purity::Unknown);
    EXPECT_FALSE(fn.meta.is_recursive);
}

// --- Add instr gets correct type ---
TEST(IRTest, AddInstrType) {
    auto r = buildIR("fn main() -> i64 { 1 + 2 }");
    auto& fn = r.ir.functions[0];
    bool found = false;
    for (auto& block : fn.blocks) {
        for (auto& instr : block.instrs) {
            if (instr.op == IROpcode::Add) {
                EXPECT_EQ(instr.type, IRType::I64);
                found = true;
            }
        }
    }
    EXPECT_TRUE(found);
}

// --- i32 arithmetic gives i32 type ---
TEST(IRTest, I32ArithType) {
    auto r = buildIR("fn add32(a: i32, b: i32) -> i32 { a + b }");
    auto& fn = r.ir.functions[0];
    bool found = false;
    for (auto& block : fn.blocks) {
        for (auto& instr : block.instrs) {
            if (instr.op == IROpcode::Add) {
                EXPECT_EQ(instr.type, IRType::I32);
                found = true;
            }
        }
    }
    EXPECT_TRUE(found);
}

// --- dumpIR includes type annotation ---
TEST(IRTest, DumpIRTypeAnnotation) {
    auto r = buildIR("fn main() -> i64 { 42 }");
    std::ostringstream out;
    dumpIR(r.ir, out);
    std::string dump = out.str();
    EXPECT_NE(dump.find(":i64"), std::string::npos);
}

// --- Call instr type matches callee return ---
TEST(IRTest, CallInstrType) {
    auto r = buildIR(
        "fn get() -> i64 { 42 }\n"
        "fn main() -> i64 { get() }"
    );
    auto& fn = r.ir.functions[1]; // main
    bool found = false;
    for (auto& block : fn.blocks) {
        for (auto& instr : block.instrs) {
            if (instr.op == IROpcode::Call) {
                EXPECT_EQ(instr.type, IRType::I64);
                found = true;
            }
        }
    }
    EXPECT_TRUE(found);
}

// --- u64 param type ---
TEST(IRTest, ParamTypeU64) {
    auto r = buildIR("fn f(x: u64) -> u64 { x }");
    auto& fn = r.ir.functions[0];
    ASSERT_EQ(fn.param_types.size(), 1u);
    EXPECT_EQ(fn.param_types[0], IRType::U64);
}

// --- dumpIR shows param types ---
TEST(IRTest, DumpIRParamTypes) {
    auto r = buildIR("fn add(a: i64, b: i64) -> i64 { a + b }");
    std::ostringstream out;
    dumpIR(r.ir, out);
    std::string dump = out.str();
    EXPECT_NE(dump.find(": i64"), std::string::npos);
}

// ===== M2.5: Purity annotation in IR dump =====

// Helper that includes purity analysis
static IRResult buildIRWithPurity(std::string src) {
    IRResult r;
    r.source = std::move(src);
    Lexer lexer(r.source, "test.kern", r.diag);
    Parser parser(lexer, r.arena, r.diag);
    Module* mod = parser.parseModule();
    EXPECT_FALSE(r.diag.hasErrors());

    TypeChecker tc(r.diag);
    tc.check(mod);

    PurityChecker pc(r.diag);
    auto purity_map = pc.analyze(mod);

    IRBuilder builder;
    r.ir = builder.build(mod, tc);

    // Apply purity metadata
    for (auto& irFn : r.ir.functions) {
        auto it = purity_map.find(std::string_view(irFn.name));
        if (it != purity_map.end()) {
            irFn.meta.purity = it->second.purity;
            irFn.meta.is_recursive = it->second.is_recursive;
        }
    }
    return r;
}

// --- Pure function shows [pure] in dump ---
TEST(IRTest, DumpIRPureAnnotation) {
    auto r = buildIRWithPurity("fn main() -> i64 { 42 }");
    std::ostringstream out;
    dumpIR(r.ir, out);
    std::string dump = out.str();
    EXPECT_NE(dump.find("[pure]"), std::string::npos);
}

// --- ImpureMut function shows [impure(mut)] ---
TEST(IRTest, DumpIRImpureMutAnnotation) {
    auto r = buildIRWithPurity(
        "fn f() -> i64 {\n"
        "    var x: i64 = 10\n"
        "    x\n"
        "}"
    );
    std::ostringstream out;
    dumpIR(r.ir, out);
    std::string dump = out.str();
    EXPECT_NE(dump.find("[impure(mut)]"), std::string::npos);
}

// --- Multiple functions with mixed purity ---
TEST(IRTest, DumpIRMixedPurity) {
    auto r = buildIRWithPurity(
        "fn pure_fn() -> i64 { 42 }\n"
        "fn impure_fn() -> i64 {\n"
        "    var x: i64 = 10\n"
        "    x\n"
        "}"
    );
    std::ostringstream out;
    dumpIR(r.ir, out);
    std::string dump = out.str();
    EXPECT_NE(dump.find("[pure]"), std::string::npos);
    EXPECT_NE(dump.find("[impure(mut)]"), std::string::npos);
}

// --- Purity metadata correctly set on IRFunction ---
TEST(IRTest, PurityMetadataSet) {
    auto r = buildIRWithPurity("fn main() -> i64 { 42 }");
    EXPECT_EQ(r.ir.functions[0].meta.purity, Purity::Pure);
}

// --- Recursive function metadata ---
TEST(IRTest, RecursiveMetadata) {
    auto r = buildIRWithPurity(
        "fn fib(n: i64) -> i64 {\n"
        "    if n <= 1 { n } else { fib(n - 1) + fib(n - 2) }\n"
        "}"
    );
    EXPECT_EQ(r.ir.functions[0].meta.purity, Purity::Pure);
    EXPECT_TRUE(r.ir.functions[0].meta.is_recursive);
}

// --- Caller of impure(mut) is still pure ---
TEST(IRTest, CallerOfImpureMutStillPure) {
    auto r = buildIRWithPurity(
        "fn impure_fn() -> i64 {\n"
        "    var x: i64 = 10\n"
        "    x\n"
        "}\n"
        "fn caller() -> i64 { impure_fn() }"
    );
    // Find caller
    for (auto& fn : r.ir.functions) {
        if (fn.name == "caller") {
            EXPECT_EQ(fn.meta.purity, Purity::Pure);
        }
    }
}

// ===== M3.3: Float IR tests =====

// --- ConstFloat ---
TEST(IRTest, FloatConst) {
    auto r = buildIR("fn main() -> f64 { 3.14 }");
    auto& fn = r.ir.functions[0];
    bool found = false;
    for (auto& block : fn.blocks) {
        for (auto& instr : block.instrs) {
            if (instr.op == IROpcode::ConstFloat) {
                EXPECT_DOUBLE_EQ(instr.imm_float, 3.14);
                EXPECT_EQ(instr.type, IRType::F64);
                found = true;
            }
        }
    }
    EXPECT_TRUE(found);
}

// --- FAdd ---
TEST(IRTest, FloatAdd) {
    auto r = buildIR("fn add(a: f64, b: f64) -> f64 { a + b }");
    EXPECT_TRUE(hasOpcode(r.ir.functions[0], IROpcode::FAdd));
}

// --- f32 add ---
TEST(IRTest, F32Add) {
    auto r = buildIR("fn add32(a: f32, b: f32) -> f32 { a + b }");
    EXPECT_TRUE(hasOpcode(r.ir.functions[0], IROpcode::FAdd));
}

// --- FCmpLt ---
TEST(IRTest, FloatCmp) {
    auto r = buildIR("fn cmp(a: f64, b: f64) -> bool { a < b }");
    EXPECT_TRUE(hasOpcode(r.ir.functions[0], IROpcode::FCmpLt));
}

// --- FNeg ---
TEST(IRTest, FloatNeg) {
    auto r = buildIR("fn main() -> f64 { -3.14 }");
    EXPECT_TRUE(hasOpcode(r.ir.functions[0], IROpcode::FNeg));
}

// --- dumpIR shows const_float ---
TEST(IRTest, FloatDump) {
    auto r = buildIR("fn main() -> f64 { 3.14 }");
    std::ostringstream out;
    dumpIR(r.ir, out);
    std::string dump = out.str();
    EXPECT_NE(dump.find("const_float"), std::string::npos);
    EXPECT_NE(dump.find("3.14"), std::string::npos);
}

// --- f64 param type ---
TEST(IRTest, F64ParamType) {
    auto r = buildIR("fn f(x: f64) -> f64 { x }");
    auto& fn = r.ir.functions[0];
    ASSERT_EQ(fn.param_types.size(), 1u);
    EXPECT_EQ(fn.param_types[0], IRType::F64);
}

// --- dumpIR no purity annotation when Unknown ---
TEST(IRTest, DumpIRNoPurityWhenUnknown) {
    auto r = buildIR("fn main() -> i64 { 42 }");
    // buildIR doesn't run purity, so meta.purity is Unknown
    std::ostringstream out;
    dumpIR(r.ir, out);
    std::string dump = out.str();
    EXPECT_EQ(dump.find("[pure]"), std::string::npos);
    EXPECT_EQ(dump.find("[impure"), std::string::npos);
}
