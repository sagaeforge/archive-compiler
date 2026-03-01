#include "kern/support/Diagnostic.h"
#include "kern/support/SourceLocation.h"
#include <gtest/gtest.h>
#include <sstream>

using namespace kern;

TEST(DiagnosticTest, NoErrorsInitially) {
    DiagnosticEngine diag;
    EXPECT_FALSE(diag.hasErrors());
}

TEST(DiagnosticTest, ReportError) {
    DiagnosticEngine diag;
    diag.error({1, 5, "test.kern"}, "unexpected token");
    EXPECT_TRUE(diag.hasErrors());
    EXPECT_EQ(diag.diagnostics().size(), 1u);
}

TEST(DiagnosticTest, ReportWarning) {
    DiagnosticEngine diag;
    diag.warning({3, 1, "test.kern"}, "var usage");
    EXPECT_FALSE(diag.hasErrors());
    EXPECT_EQ(diag.diagnostics().size(), 1u);
}

TEST(DiagnosticTest, PrintFormatting) {
    DiagnosticEngine diag;
    diag.error({2, 10, "test.kern"}, "type mismatch");
    std::ostringstream out;
    diag.printAll(out);
    std::string output = out.str();
    EXPECT_NE(output.find("error"), std::string::npos);
    EXPECT_NE(output.find("test.kern:2:10"), std::string::npos);
    EXPECT_NE(output.find("type mismatch"), std::string::npos);
}

// --- Note-level diagnostic ---
TEST(DiagnosticTest, NoteLevelDiagnostic) {
    DiagnosticEngine diag;
    diag.report(DiagLevel::Note, {1, 1, "test.kern"}, "additional info");
    EXPECT_FALSE(diag.hasErrors());
    EXPECT_EQ(diag.diagnostics().size(), 1u);
    std::ostringstream out;
    diag.printAll(out);
    EXPECT_NE(out.str().find("note"), std::string::npos);
    EXPECT_NE(out.str().find("additional info"), std::string::npos);
}

// --- Unknown location (line 0) ---
TEST(DiagnosticTest, UnknownLocation) {
    DiagnosticEngine diag;
    diag.error({0, 0, "test.kern"}, "something went wrong");
    EXPECT_TRUE(diag.hasErrors());
    std::ostringstream out;
    diag.printAll(out);
    std::string output = out.str();
    // When line == 0, location should not be printed
    EXPECT_EQ(output.find("test.kern:0:0"), std::string::npos);
    EXPECT_NE(output.find("something went wrong"), std::string::npos);
}

// --- SourceLocation::unknown() factory ---
TEST(DiagnosticTest, SourceLocationUnknown) {
    auto loc = SourceLocation::unknown();
    EXPECT_EQ(loc.line, 0u);
    EXPECT_EQ(loc.col, 0u);
    EXPECT_EQ(loc.filename, "<unknown>");
}
