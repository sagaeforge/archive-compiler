#include <gtest/gtest.h>
#include "kern/ide/IDEContext.h"
#include "kern/ide/CompletionProvider.h"
#include "kern/ide/HoverProvider.h"
#include "kern/ide/DefinitionProvider.h"
#include "kern/ide/ReferencesProvider.h"
#include "kern/ide/SemanticTokens.h"
#include "kern/ide/DiagnosticProvider.h"
#include "kern/support/CompilationContext.h"

using namespace kern;

static const char* SIMPLE_SOURCE =
    "fn add(a: i64, b: i64) -> i64 { a + b }\n"
    "fn main() -> i64 { add(20, 22) }\n";

static const char* STRUCT_SOURCE =
    "struct Point { val x: i64, val y: i64 }\n"
    "fn make() -> Point { Point { x: 1, y: 2 } }\n";

static const char* ENUM_SOURCE =
    "enum Color { Red, Green, Blue }\n"
    "fn pick() -> i64 { Color.Red }\n";

// ============================================================================
// IDEContext tests
// ============================================================================

TEST(IDEContextTest, OpenAndCloseFile) {
    CompilationContext ctx;
    IDEContext ide(ctx);

    EXPECT_FALSE(ide.hasFile("test.kern"));
    EXPECT_EQ(ide.fileCount(), 0u);

    ide.openFile("test.kern", SIMPLE_SOURCE);
    EXPECT_TRUE(ide.hasFile("test.kern"));
    EXPECT_EQ(ide.fileCount(), 1u);
    EXPECT_EQ(ide.fileVersion("test.kern"), 1u);

    ide.closeFile("test.kern");
    EXPECT_FALSE(ide.hasFile("test.kern"));
    EXPECT_EQ(ide.fileCount(), 0u);
}

TEST(IDEContextTest, UpdateFileIncrementsVersion) {
    CompilationContext ctx;
    IDEContext ide(ctx);

    ide.openFile("test.kern", SIMPLE_SOURCE);
    EXPECT_EQ(ide.fileVersion("test.kern"), 1u);

    ide.updateFile("test.kern", SIMPLE_SOURCE);
    EXPECT_EQ(ide.fileVersion("test.kern"), 2u);
}

TEST(IDEContextTest, GetContent) {
    CompilationContext ctx;
    IDEContext ide(ctx);

    ide.openFile("test.kern", SIMPLE_SOURCE);
    auto content = ide.getContent("test.kern");
    EXPECT_EQ(content, SIMPLE_SOURCE);
}

TEST(IDEContextTest, GetContentMissing) {
    CompilationContext ctx;
    IDEContext ide(ctx);

    EXPECT_TRUE(ide.getContent("missing.kern").empty());
}

TEST(IDEContextTest, GetAST) {
    CompilationContext ctx;
    ctx.diag.setSource(SIMPLE_SOURCE);
    IDEContext ide(ctx);

    ide.openFile("test.kern", SIMPLE_SOURCE);
    const Module* ast = ide.getAST("test.kern");
    EXPECT_NE(ast, nullptr);
}

TEST(IDEContextTest, GetHIR) {
    CompilationContext ctx;
    ctx.diag.setSource(SIMPLE_SOURCE);
    IDEContext ide(ctx);

    ide.openFile("test.kern", SIMPLE_SOURCE);
    const HIRModule* hir = ide.getHIR("test.kern");
    EXPECT_NE(hir, nullptr);
    EXPECT_GE(hir->fn_count, 2u);  // add + main
}

TEST(IDEContextTest, GetHIRMissing) {
    CompilationContext ctx;
    IDEContext ide(ctx);
    EXPECT_EQ(ide.getHIR("missing.kern"), nullptr);
}

TEST(IDEContextTest, LazyRebuildOnUpdate) {
    CompilationContext ctx;
    ctx.diag.setSource(SIMPLE_SOURCE);
    IDEContext ide(ctx);

    ide.openFile("test.kern", SIMPLE_SOURCE);
    const HIRModule* hir1 = ide.getHIR("test.kern");
    EXPECT_NE(hir1, nullptr);

    // Update invalidates cache
    ide.updateFile("test.kern", SIMPLE_SOURCE);
    // getHIR triggers rebuild
    const HIRModule* hir2 = ide.getHIR("test.kern");
    EXPECT_NE(hir2, nullptr);
}

TEST(IDEContextTest, FileVersionNonExistent) {
    CompilationContext ctx;
    IDEContext ide(ctx);
    EXPECT_EQ(ide.fileVersion("nope.kern"), 0u);
}

TEST(IDEContextTest, UpdateNonExistentOpens) {
    CompilationContext ctx;
    IDEContext ide(ctx);
    ide.updateFile("new.kern", SIMPLE_SOURCE);
    EXPECT_TRUE(ide.hasFile("new.kern"));
}

// ============================================================================
// CompletionProvider tests
// ============================================================================

TEST(CompletionTest, KeywordCompletion) {
    CompilationContext ctx;
    ctx.diag.setSource("fn ");
    IDEContext ide(ctx);
    ide.openFile("test.kern", "fn ");

    CompletionProvider provider;
    // At column 4 (after "fn "), prefix is empty → all completions
    auto items = provider.complete(ide, "test.kern", 1, 4);
    EXPECT_GT(items.size(), 0u);

    // Check that keywords are included
    bool has_fn = false;
    for (const auto& item : items) {
        if (item.label == "fn") has_fn = true;
    }
    EXPECT_TRUE(has_fn);
}

TEST(CompletionTest, FunctionCompletion) {
    CompilationContext ctx;
    ctx.diag.setSource(SIMPLE_SOURCE);
    IDEContext ide(ctx);
    ide.openFile("test.kern", SIMPLE_SOURCE);

    CompletionProvider provider;
    // Prefix "ad" should match "add"
    auto items = provider.complete(ide, "test.kern", 2, 22);  // "add" starts at col 21
    bool has_add = false;
    for (const auto& item : items) {
        if (item.label == "add" && item.kind == CompletionItem::Function) {
            has_add = true;
        }
    }
    EXPECT_TRUE(has_add);
}

TEST(CompletionTest, TypeCompletion) {
    CompilationContext ctx;
    ctx.diag.setSource(STRUCT_SOURCE);
    IDEContext ide(ctx);
    ide.openFile("test.kern", STRUCT_SOURCE);

    CompletionProvider provider;
    auto items = provider.complete(ide, "test.kern", 2, 1);  // start of line, empty prefix
    bool has_point = false;
    for (const auto& item : items) {
        if (item.label == "Point" && item.kind == CompletionItem::Type) {
            has_point = true;
        }
    }
    EXPECT_TRUE(has_point);
}

TEST(CompletionTest, EmptyFile) {
    CompilationContext ctx;
    IDEContext ide(ctx);

    CompletionProvider provider;
    auto items = provider.complete(ide, "missing.kern", 1, 1);
    EXPECT_TRUE(items.empty());
}

TEST(CompletionTest, PrefixFiltering) {
    CompilationContext ctx;
    ctx.diag.setSource(SIMPLE_SOURCE);
    IDEContext ide(ctx);
    ide.openFile("test.kern", SIMPLE_SOURCE);

    CompletionProvider provider;
    // After "ma" at line 2 col 5 (fn main...), prefix should be "ma"
    // "main", "match", "make" keywords should show
    auto items = provider.complete(ide, "test.kern", 2, 5);
    bool has_match = false;
    for (const auto& item : items) {
        if (item.label == "match") has_match = true;
    }
    EXPECT_TRUE(has_match);
}

// ============================================================================
// HoverProvider tests
// ============================================================================

TEST(HoverTest, HoverOnFunction) {
    CompilationContext ctx;
    ctx.diag.setSource(SIMPLE_SOURCE);
    IDEContext ide(ctx);
    ide.openFile("test.kern", SIMPLE_SOURCE);

    HoverProvider provider;
    // "add" is at line 1, col 4
    auto result = provider.hover(ide, "test.kern", 1, 4);
    ASSERT_TRUE(result.has_value());
    EXPECT_NE(result->type_info.find("fn add"), std::string::npos);
    EXPECT_NE(result->type_info.find("i64"), std::string::npos);
}

TEST(HoverTest, HoverOnStruct) {
    CompilationContext ctx;
    ctx.diag.setSource(STRUCT_SOURCE);
    IDEContext ide(ctx);
    ide.openFile("test.kern", STRUCT_SOURCE);

    HoverProvider provider;
    // "Point" at line 1, col 8
    auto result = provider.hover(ide, "test.kern", 1, 8);
    ASSERT_TRUE(result.has_value());
    EXPECT_NE(result->type_info.find("struct Point"), std::string::npos);
}

TEST(HoverTest, HoverOnEnum) {
    CompilationContext ctx;
    ctx.diag.setSource(ENUM_SOURCE);
    IDEContext ide(ctx);
    ide.openFile("test.kern", ENUM_SOURCE);

    HoverProvider provider;
    auto result = provider.hover(ide, "test.kern", 1, 6);  // "Color"
    ASSERT_TRUE(result.has_value());
    EXPECT_NE(result->type_info.find("enum Color"), std::string::npos);
}

TEST(HoverTest, HoverOnNothing) {
    CompilationContext ctx;
    ctx.diag.setSource(SIMPLE_SOURCE);
    IDEContext ide(ctx);
    ide.openFile("test.kern", SIMPLE_SOURCE);

    HoverProvider provider;
    // Hover on empty area
    auto result = provider.hover(ide, "test.kern", 1, 1);  // "fn" keyword
    EXPECT_FALSE(result.has_value());
}

TEST(HoverTest, HoverMissingFile) {
    CompilationContext ctx;
    IDEContext ide(ctx);

    HoverProvider provider;
    auto result = provider.hover(ide, "missing.kern", 1, 1);
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// DefinitionProvider tests
// ============================================================================

TEST(DefinitionTest, FindFunctionDef) {
    CompilationContext ctx;
    ctx.diag.setSource(SIMPLE_SOURCE);
    IDEContext ide(ctx);
    ide.openFile("test.kern", SIMPLE_SOURCE);

    DefinitionProvider provider;
    // "add" at line 2, col 22 (the call site)
    auto result = provider.findDefinition(ide, "test.kern", 2, 22);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->name, "add");
    EXPECT_EQ(result->location.line, 1u);
}

TEST(DefinitionTest, FindStructDef) {
    CompilationContext ctx;
    ctx.diag.setSource(STRUCT_SOURCE);
    IDEContext ide(ctx);
    ide.openFile("test.kern", STRUCT_SOURCE);

    DefinitionProvider provider;
    // "Point" at line 2 in return type
    auto result = provider.findDefinition(ide, "test.kern", 2, 16);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->name, "Point");
}

TEST(DefinitionTest, FindNothing) {
    CompilationContext ctx;
    ctx.diag.setSource(SIMPLE_SOURCE);
    IDEContext ide(ctx);
    ide.openFile("test.kern", SIMPLE_SOURCE);

    DefinitionProvider provider;
    auto result = provider.findDefinition(ide, "test.kern", 1, 1);
    EXPECT_FALSE(result.has_value());
}

TEST(DefinitionTest, MissingFile) {
    CompilationContext ctx;
    IDEContext ide(ctx);

    DefinitionProvider provider;
    auto result = provider.findDefinition(ide, "missing.kern", 1, 1);
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// ReferencesProvider tests
// ============================================================================

TEST(ReferencesTest, FindFunctionReferences) {
    CompilationContext ctx;
    ctx.diag.setSource(SIMPLE_SOURCE);
    IDEContext ide(ctx);
    ide.openFile("test.kern", SIMPLE_SOURCE);

    ReferencesProvider provider;
    // "add" is defined at line 1 and called at line 2
    auto refs = provider.findReferences(ide, "test.kern", 1, 4);
    EXPECT_GE(refs.size(), 1u);  // at least definition

    bool has_def = false;
    for (const auto& r : refs) {
        if (r.is_definition) has_def = true;
    }
    EXPECT_TRUE(has_def);
}

TEST(ReferencesTest, ExcludeDefinition) {
    CompilationContext ctx;
    ctx.diag.setSource(SIMPLE_SOURCE);
    IDEContext ide(ctx);
    ide.openFile("test.kern", SIMPLE_SOURCE);

    ReferencesProvider provider;
    auto refs = provider.findReferences(ide, "test.kern", 1, 4, false);
    for (const auto& r : refs) {
        EXPECT_FALSE(r.is_definition);
    }
}

TEST(ReferencesTest, EmptyForUnknown) {
    CompilationContext ctx;
    ctx.diag.setSource(SIMPLE_SOURCE);
    IDEContext ide(ctx);
    ide.openFile("test.kern", SIMPLE_SOURCE);

    ReferencesProvider provider;
    auto refs = provider.findReferences(ide, "test.kern", 1, 1);  // "fn" keyword
    EXPECT_TRUE(refs.empty());
}

TEST(ReferencesTest, MissingFile) {
    CompilationContext ctx;
    IDEContext ide(ctx);

    ReferencesProvider provider;
    auto refs = provider.findReferences(ide, "missing.kern", 1, 1);
    EXPECT_TRUE(refs.empty());
}

// ============================================================================
// SemanticTokens tests
// ============================================================================

TEST(SemanticTokensTest, TokenizesKeywords) {
    CompilationContext ctx;
    ctx.diag.setSource(SIMPLE_SOURCE);
    IDEContext ide(ctx);
    ide.openFile("test.kern", SIMPLE_SOURCE);

    SemanticTokensProvider provider;
    auto tokens = provider.tokenize(ide, "test.kern");
    EXPECT_GT(tokens.size(), 0u);

    bool has_keyword = false;
    for (const auto& t : tokens) {
        if (t.type == SemanticTokenType::Keyword) has_keyword = true;
    }
    EXPECT_TRUE(has_keyword);
}

TEST(SemanticTokensTest, TokenizesFunction) {
    CompilationContext ctx;
    ctx.diag.setSource(SIMPLE_SOURCE);
    IDEContext ide(ctx);
    ide.openFile("test.kern", SIMPLE_SOURCE);

    SemanticTokensProvider provider;
    auto tokens = provider.tokenize(ide, "test.kern");

    bool has_fn = false;
    for (const auto& t : tokens) {
        if (t.type == SemanticTokenType::Function) has_fn = true;
    }
    EXPECT_TRUE(has_fn);
}

TEST(SemanticTokensTest, TokenizesNumbers) {
    CompilationContext ctx;
    ctx.diag.setSource(SIMPLE_SOURCE);
    IDEContext ide(ctx);
    ide.openFile("test.kern", SIMPLE_SOURCE);

    SemanticTokensProvider provider;
    auto tokens = provider.tokenize(ide, "test.kern");

    bool has_number = false;
    for (const auto& t : tokens) {
        if (t.type == SemanticTokenType::Number) has_number = true;
    }
    EXPECT_TRUE(has_number);
}

TEST(SemanticTokensTest, TokenizesOperators) {
    CompilationContext ctx;
    ctx.diag.setSource(SIMPLE_SOURCE);
    IDEContext ide(ctx);
    ide.openFile("test.kern", SIMPLE_SOURCE);

    SemanticTokensProvider provider;
    auto tokens = provider.tokenize(ide, "test.kern");

    bool has_op = false;
    for (const auto& t : tokens) {
        if (t.type == SemanticTokenType::Operator) has_op = true;
    }
    EXPECT_TRUE(has_op);
}

TEST(SemanticTokensTest, TokenizesTypeNames) {
    CompilationContext ctx;
    ctx.diag.setSource(STRUCT_SOURCE);
    IDEContext ide(ctx);
    ide.openFile("test.kern", STRUCT_SOURCE);

    SemanticTokensProvider provider;
    auto tokens = provider.tokenize(ide, "test.kern");

    bool has_type = false;
    for (const auto& t : tokens) {
        if (t.type == SemanticTokenType::Type) has_type = true;
    }
    EXPECT_TRUE(has_type);
}

TEST(SemanticTokensTest, EmptyFile) {
    CompilationContext ctx;
    IDEContext ide(ctx);

    SemanticTokensProvider provider;
    auto tokens = provider.tokenize(ide, "missing.kern");
    EXPECT_TRUE(tokens.empty());
}

TEST(SemanticTokensTest, TypeNameMapping) {
    EXPECT_STREQ(semanticTokenTypeName(SemanticTokenType::Function), "function");
    EXPECT_STREQ(semanticTokenTypeName(SemanticTokenType::Variable), "variable");
    EXPECT_STREQ(semanticTokenTypeName(SemanticTokenType::Keyword), "keyword");
    EXPECT_STREQ(semanticTokenTypeName(SemanticTokenType::Number), "number");
    EXPECT_STREQ(semanticTokenTypeName(SemanticTokenType::String), "string");
    EXPECT_STREQ(semanticTokenTypeName(SemanticTokenType::Operator), "operator");
    EXPECT_STREQ(semanticTokenTypeName(SemanticTokenType::Type), "type");
    EXPECT_STREQ(semanticTokenTypeName(SemanticTokenType::Parameter), "parameter");
    EXPECT_STREQ(semanticTokenTypeName(SemanticTokenType::Comment), "comment");
    EXPECT_STREQ(semanticTokenTypeName(SemanticTokenType::EnumMember), "enumMember");
}

// ============================================================================
// DiagnosticProvider Tests
// ============================================================================

TEST(DiagnosticProviderTest, NoDiagnosticsForValidCode) {
    CompilationContext ctx;
    IDEContext ide(ctx);
    ide.openFile("valid.kern", SIMPLE_SOURCE);

    DiagnosticProvider provider;
    auto diags = provider.diagnose(ide, "valid.kern");
    EXPECT_TRUE(diags.empty());
}

TEST(DiagnosticProviderTest, ReportsTypeError) {
    CompilationContext ctx;
    IDEContext ide(ctx);
    ide.openFile("err.kern", "fn main() -> i64 { true + 1 }");

    DiagnosticProvider provider;
    auto diags = provider.diagnose(ide, "err.kern");
    EXPECT_FALSE(diags.empty());
    EXPECT_EQ(diags[0].severity, DiagLevel::Error);
}

TEST(DiagnosticProviderTest, EmptyForUnknownFile) {
    CompilationContext ctx;
    IDEContext ide(ctx);

    DiagnosticProvider provider;
    auto diags = provider.diagnose(ide, "unknown.kern");
    EXPECT_TRUE(diags.empty());
}

TEST(DiagnosticProviderTest, DiagnosticHasLocation) {
    CompilationContext ctx;
    IDEContext ide(ctx);
    ide.openFile("loc.kern", "fn main() -> i64 { undefined_fn() }");

    DiagnosticProvider provider;
    auto diags = provider.diagnose(ide, "loc.kern");
    EXPECT_FALSE(diags.empty());
    EXPECT_GT(diags[0].loc.line, 0u);
}
