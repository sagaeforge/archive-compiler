#include "kern/support/Diagnostic.h"
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
