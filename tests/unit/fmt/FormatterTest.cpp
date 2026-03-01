#include "kern/fmt/Formatter.h"
#include "kern/lexer/Lexer.h"
#include "kern/parser/Parser.h"
#include "kern/support/Arena.h"
#include "kern/support/Diagnostic.h"
#include <gtest/gtest.h>
#include <sstream>

namespace kern {

class FormatterTest : public ::testing::Test {
protected:
    Arena arena;
    DiagnosticEngine diag;

    std::string format(const std::string& source) {
        diag.setSource(source);
        Lexer lexer(source, "test.kern", diag);
        Parser parser(lexer, arena, diag);
        auto* mod = parser.parseModule();
        EXPECT_FALSE(diag.hasErrors());
        std::ostringstream out;
        Formatter fmt(out);
        fmt.formatModule(mod);
        return out.str();
    }
};

TEST_F(FormatterTest, SimpleFunction) {
    auto result = format("fn main() -> i64 { 42 }");
    EXPECT_TRUE(result.find("fn main() -> i64") != std::string::npos) << result;
    EXPECT_TRUE(result.find("42") != std::string::npos) << result;
}

TEST_F(FormatterTest, FunctionWithParams) {
    auto result = format("fn add(a: i64, b: i64) -> i64 { a + b }");
    EXPECT_TRUE(result.find("fn add(a: i64, b: i64) -> i64") != std::string::npos) << result;
}

TEST_F(FormatterTest, StructDecl) {
    auto result = format("struct Point { x: i64, y: i64 }");
    EXPECT_TRUE(result.find("struct Point") != std::string::npos) << result;
    EXPECT_TRUE(result.find("x: i64") != std::string::npos) << result;
    EXPECT_TRUE(result.find("y: i64") != std::string::npos) << result;
}

TEST_F(FormatterTest, EnumDecl) {
    auto result = format("enum Color { Red, Green, Blue }");
    EXPECT_TRUE(result.find("enum Color") != std::string::npos) << result;
    EXPECT_TRUE(result.find("Red") != std::string::npos) << result;
}

TEST_F(FormatterTest, UnionDecl) {
    auto result = format("union Shape { Circle(i64), Square(i64) }");
    EXPECT_TRUE(result.find("union Shape") != std::string::npos) << result;
    EXPECT_TRUE(result.find("Circle(i64)") != std::string::npos) << result;
}

TEST_F(FormatterTest, ValDecl) {
    auto result = format("fn main() -> i64 { val x: i64 = 42\n x }");
    EXPECT_TRUE(result.find("val x: i64 = 42") != std::string::npos) << result;
}

TEST_F(FormatterTest, VarDecl) {
    auto result = format("fn main() -> i64 { var x: i64 = 10\n x }");
    EXPECT_TRUE(result.find("var x: i64 = 10") != std::string::npos) << result;
}

TEST_F(FormatterTest, IfExpr) {
    auto result = format("fn f(x: i64) -> i64 { if x > 0 { 1 } else { 0 } }");
    EXPECT_TRUE(result.find("if x > 0") != std::string::npos) << result;
    EXPECT_TRUE(result.find("else") != std::string::npos) << result;
}

TEST_F(FormatterTest, MatchExpr) {
    auto result = format("fn f(x: bool) -> i64 { match x { true => 1\n false => 0 } }");
    EXPECT_TRUE(result.find("match x") != std::string::npos) << result;
    EXPECT_TRUE(result.find("true => 1") != std::string::npos) << result;
    EXPECT_TRUE(result.find("false => 0") != std::string::npos) << result;
}

TEST_F(FormatterTest, Intrinsic) {
    auto result = format("fn print(x: i64) -> Unit = intrinsic");
    EXPECT_TRUE(result.find("= intrinsic") != std::string::npos) << result;
}

TEST_F(FormatterTest, PtrType) {
    auto result = format("fn f(p: Ptr<i64>) -> i64 { (*p) }");
    EXPECT_TRUE(result.find("Ptr<i64>") != std::string::npos) << result;
}

TEST_F(FormatterTest, PtrVarType) {
    auto result = format("fn f(p: Ptr<var i64>) -> Unit { *p = 42 }");
    EXPECT_TRUE(result.find("Ptr<var i64>") != std::string::npos) << result;
}

TEST_F(FormatterTest, StringLit) {
    auto result = format("fn f() -> String { \"hello\" }");
    EXPECT_TRUE(result.find("\"hello\"") != std::string::npos) << result;
}

TEST_F(FormatterTest, CallExpr) {
    auto result = format("fn f() -> i64 { add(1, 2) }");
    EXPECT_TRUE(result.find("add(1, 2)") != std::string::npos) << result;
}

TEST_F(FormatterTest, IndentWidth) {
    auto result = format("fn main() -> i64 { val x: i64 = 42\n x }");
    // Default 4-space indent
    EXPECT_TRUE(result.find("    val x") != std::string::npos) << result;
}

} // namespace kern
