#include "kern/pipeline/CompilerPipeline.h"
#include <gtest/gtest.h>
#include <sstream>

namespace kern {

class PipelineTest : public ::testing::Test {
protected:
    CompilationContext ctx;

    int compile(const char* source, CompileOptions opts = {}) {
        std::ostringstream out, err;
        CompilerPipeline pipeline(ctx);
        opts.input_file = "test.kern";
        opts.asm_only = true;  // don't actually assemble/link
        return pipeline.run(source, opts, out, err);
    }

    std::string compileToAsm(const char* source) {
        CompileOptions opts;
        opts.input_file = "test.kern";
        opts.asm_only = true;
        std::ostringstream out, err;
        CompilerPipeline pipeline(ctx);
        pipeline.run(source, opts, out, err);
        return out.str();
    }

    std::string getDumpOutput(const char* source, CompileOptions opts) {
        std::ostringstream out, err;
        opts.input_file = "test.kern";
        CompilerPipeline pipeline(ctx);
        pipeline.run(source, opts, out, err);
        return out.str();
    }

    std::string getErrorOutput(const char* source) {
        CompileOptions opts;
        opts.input_file = "test.kern";
        opts.asm_only = true;
        std::ostringstream out, err;
        CompilerPipeline pipeline(ctx);
        pipeline.run(source, opts, out, err);
        return err.str();
    }
};

// ============================================================================
// Basic pipeline success/failure
// ============================================================================

TEST_F(PipelineTest, SimpleProgram) {
    EXPECT_EQ(compile("fn main() -> i64 { 42 }"), 0);
}

TEST_F(PipelineTest, EmptySource) {
    // No main function — should succeed (empty module)
    EXPECT_EQ(compile(""), 0);
}

TEST_F(PipelineTest, ParseError) {
    EXPECT_NE(compile("fn main( { }"), 0);
}

TEST_F(PipelineTest, TypeErrorUndeclaredIdent) {
    EXPECT_NE(compile("fn main() -> i64 { foo }"), 0);
}

TEST_F(PipelineTest, TypeErrorMismatch) {
    EXPECT_NE(compile("fn main() -> i64 { true }"), 0);
}

// ============================================================================
// Dump flags
// ============================================================================

TEST_F(PipelineTest, DumpTokens) {
    CompileOptions opts;
    opts.dump_tokens = true;
    auto output = getDumpOutput("fn main() -> i64 { 42 }", opts);
    EXPECT_NE(output.find("fn"), std::string::npos);
    EXPECT_NE(output.find("main"), std::string::npos);
    EXPECT_NE(output.find("42"), std::string::npos);
}

TEST_F(PipelineTest, DumpAST) {
    CompileOptions opts;
    opts.dump_ast = true;
    auto output = getDumpOutput("fn main() -> i64 { 42 }", opts);
    EXPECT_NE(output.find("main"), std::string::npos);
}

TEST_F(PipelineTest, DumpHIR) {
    CompileOptions opts;
    opts.dump_hir = true;
    auto output = getDumpOutput("fn main() -> i64 { 42 }", opts);
    EXPECT_NE(output.find("fn main"), std::string::npos);
    EXPECT_NE(output.find("i64"), std::string::npos);
}

TEST_F(PipelineTest, DumpLIR) {
    CompileOptions opts;
    opts.dump_lir = true;
    auto output = getDumpOutput("fn main() -> i64 { 42 }", opts);
    EXPECT_NE(output.find("main"), std::string::npos);
}

TEST_F(PipelineTest, DumpMachIR) {
    CompileOptions opts;
    opts.dump_machir = true;
    auto output = getDumpOutput("fn main() -> i64 { 42 }", opts);
    EXPECT_NE(output.find("main"), std::string::npos);
}

TEST_F(PipelineTest, DumpPurity) {
    CompileOptions opts;
    opts.dump_purity = true;
    auto output = getDumpOutput(R"(
        fn pure_fn(x: i64) -> i64 { x + 1 }
        fn main() -> i64 { pure_fn(41) }
    )", opts);
    EXPECT_NE(output.find("pure"), std::string::npos);
}

// ============================================================================
// Pipeline stages (various language features)
// ============================================================================

TEST_F(PipelineTest, ArithmeticOps) {
    EXPECT_EQ(compile("fn main() -> i64 { 20 + 22 }"), 0);
}

TEST_F(PipelineTest, FunctionCalls) {
    EXPECT_EQ(compile(R"(
        fn add(a: i64, b: i64) -> i64 { a + b }
        fn main() -> i64 { add(20, 22) }
    )"), 0);
}

TEST_F(PipelineTest, IfElse) {
    EXPECT_EQ(compile(R"(
        fn main() -> i64 { if true { 42 } else { 0 } }
    )"), 0);
}

TEST_F(PipelineTest, Struct) {
    EXPECT_EQ(compile(R"(
        struct Point { x: i64, y: i64 }
        fn main() -> i64 {
            val p: Point = Point { x: 10, y: 32 }
            p.x + p.y
        }
    )"), 0);
}

TEST_F(PipelineTest, Union) {
    EXPECT_EQ(compile(R"(
        union Option { None, Some(i64) }
        fn main() -> i64 {
            val a: Option = Option::Some(42)
            match a { None => 0, Some(v) => v }
        }
    )"), 0);
}

TEST_F(PipelineTest, GenericFunction) {
    EXPECT_EQ(compile(R"(
        fn identity<T>(x: T) -> T { x }
        fn main() -> i64 { identity(42) }
    )"), 0);
}

TEST_F(PipelineTest, Lambda) {
    EXPECT_EQ(compile(R"(
        fn apply(f: fn(i64) -> i64, x: i64) -> i64 { f(x) }
        fn main() -> i64 {
            val double: fn(i64) -> i64 = { x: i64 => x * 2 }
            apply(double, 21)
        }
    )"), 0);
}

TEST_F(PipelineTest, ClosureCapture) {
    EXPECT_EQ(compile(R"(
        fn main() -> i64 {
            val a: i64 = 10
            val f: fn(i64) -> i64 = { x: i64 => x + a }
            f(32)
        }
    )"), 0);
}

TEST_F(PipelineTest, Loop) {
    EXPECT_EQ(compile(R"(
        fn main() -> i64 {
            loop(i = 0, sum = 0) {
                if i >= 10 { break sum }
                continue(i + 1, sum + i)
            }
        }
    )"), 0);
}

TEST_F(PipelineTest, PatternMatch) {
    EXPECT_EQ(compile(R"(
        fn main() -> i64 {
            match 42 {
                0 => 0,
                _ => 42
            }
        }
    )"), 0);
}

TEST_F(PipelineTest, Pointer) {
    EXPECT_EQ(compile(R"(
        fn main() -> i64 {
            var x: i64 = 42
            val p: Ptr<var i64> = &var x
            *p
        }
    )"), 0);
}

TEST_F(PipelineTest, StringLiteral) {
    EXPECT_EQ(compile(R"(
        fn main() -> i64 {
            val s: String = "hello"
            s.len as i64
        }
    )"), 0);
}

TEST_F(PipelineTest, FloatArith) {
    EXPECT_EQ(compile(R"(
        fn main() -> i64 {
            val a: f64 = 3.14
            val b: f64 = 2.86
            if a + b == 6.0 { 42 } else { 0 }
        }
    )"), 0);
}

TEST_F(PipelineTest, TailCall) {
    EXPECT_EQ(compile(R"(
        fn count_down(n: i64) -> i64 {
            if n <= 0 { 0 } else { count_down(n - 1) }
        }
        fn main() -> i64 { count_down(100) }
    )"), 0);
}

// ============================================================================
// Error propagation
// ============================================================================

TEST_F(PipelineTest, ErrorMessageContainsLocation) {
    auto err = getErrorOutput("fn main() -> i64 { foo }");
    EXPECT_NE(err.find("test.kern"), std::string::npos);
}

TEST_F(PipelineTest, MultipleErrors) {
    auto err = getErrorOutput(R"(
        fn main() -> i64 { foo + bar }
    )");
    // Should contain at least 2 error messages
    size_t first = err.find("error");
    ASSERT_NE(first, std::string::npos);
    size_t second = err.find("error", first + 5);
    EXPECT_NE(second, std::string::npos);
}

// ============================================================================
// CompileOptions defaults
// ============================================================================

TEST_F(PipelineTest, DefaultOptions) {
    CompileOptions opts;
    EXPECT_EQ(opts.output_file, "a.out");
    EXPECT_FALSE(opts.asm_only);
    EXPECT_FALSE(opts.dump_tokens);
    EXPECT_FALSE(opts.dump_ast);
    EXPECT_FALSE(opts.dump_hir);
    EXPECT_FALSE(opts.dump_lir);
    EXPECT_FALSE(opts.dump_machir);
    EXPECT_FALSE(opts.dump_purity);
    EXPECT_FALSE(opts.freestanding);
    EXPECT_TRUE(opts.linker_script.empty());
}

} // namespace kern
