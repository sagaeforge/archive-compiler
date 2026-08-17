#include <04_parsing/ast/core/AST.hpp>
#include <gtest/gtest.h>

using namespace nugdev::ast;

class ImportExportTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Setup code if needed
  }
};

// ImportStatement Tests
TEST_F(ImportExportTest, ImportStatementBasic) {
  auto import = std::make_unique<ImportStatement>("std.io");

  ASSERT_NE(import, nullptr);
  EXPECT_EQ(import->get_node_type(), NodeType::IMPORT_STATEMENT);
  EXPECT_EQ(import->get_module_path(), "std.io");
  EXPECT_FALSE(import->has_alias());
  EXPECT_EQ(import->get_alias(), "");
  EXPECT_EQ(import->to_string(), "ImportStatement(\"std.io\")");
}

TEST_F(ImportExportTest, ImportStatementWithAlias) {
  auto import = std::make_unique<ImportStatement>("utils.mathematics", "math");

  EXPECT_EQ(import->get_module_path(), "utils.mathematics");
  EXPECT_TRUE(import->has_alias());
  EXPECT_EQ(import->get_alias(), "math");
  EXPECT_EQ(import->to_string(),
            "ImportStatement(\"utils.mathematics\" as math)");
}

TEST_F(ImportExportTest, ImportStatementLongPath) {
  auto import =
      std::make_unique<ImportStatement>("org.company.project.module.submodule");

  EXPECT_EQ(import->get_module_path(), "org.company.project.module.submodule");
  EXPECT_FALSE(import->has_alias());
}

TEST_F(ImportExportTest, ImportStatementEmptyAlias) {
  auto import = std::make_unique<ImportStatement>("std.collections", "");

  EXPECT_EQ(import->get_module_path(), "std.collections");
  EXPECT_FALSE(import->has_alias());
  EXPECT_EQ(import->get_alias(), "");
}

// ExportStatement Tests
TEST_F(ImportExportTest, ExportStatementVariable) {
  // Create a variable declaration to export
  auto type = std::make_unique<SimpleType>("int");
  auto varDecl = std::make_unique<VariableDeclaration>(
      VariableDeclaration::Mutability::LET, "PUBLIC_CONSTANT", std::move(type));

  auto exportStmt = std::make_unique<ExportStatement>(std::move(varDecl));

  ASSERT_NE(exportStmt, nullptr);
  EXPECT_EQ(exportStmt->get_node_type(), NodeType::EXPORT_STATEMENT);
  EXPECT_EQ(exportStmt->get_statement().get_node_type(),
            NodeType::VARIABLE_DECLARATION);
}

TEST_F(ImportExportTest, ExportStatementFunction) {
  // Create a function declaration to export
  std::vector<std::unique_ptr<Parameter>> params;
  auto paramType = std::make_unique<SimpleType>("string");
  params.push_back(std::make_unique<Parameter>(
      Parameter::Mutability::LET, "message", std::move(paramType)));

  auto returnType = std::make_unique<SimpleType>("void");
  auto funcDecl = std::make_unique<FunctionDeclaration>(
      "log", std::move(params), std::move(returnType));

  auto exportStmt = std::make_unique<ExportStatement>(std::move(funcDecl));

  EXPECT_EQ(exportStmt->get_statement().get_node_type(),
            NodeType::FUNCTION_DECLARATION);
}

TEST_F(ImportExportTest, ExportStatementStruct) {
  // Create struct fields
  std::vector<std::unique_ptr<StructField>> fields;

  auto xType = std::make_unique<SimpleType>("float");
  auto xField = std::make_unique<StructField>("x", std::move(xType));
  fields.push_back(std::move(xField));

  auto yType = std::make_unique<SimpleType>("float");
  auto yField = std::make_unique<StructField>("y", std::move(yType));
  fields.push_back(std::move(yField));

  // Create struct declaration with fields
  auto structDecl =
      std::make_unique<StructDeclaration>("Point", std::move(fields));
  auto exportStmt = std::make_unique<ExportStatement>(std::move(structDecl));

  EXPECT_EQ(exportStmt->get_statement().get_node_type(),
            NodeType::STRUCT_DECLARATION);
}

TEST_F(ImportExportTest, ExportStatementInterface) {
  // Create interface members
  std::vector<std::unique_ptr<InterfaceDeclaration::Member>> members;

  auto returnType = std::make_unique<SimpleType>("void");
  auto member = std::make_unique<InterfaceDeclaration::Member>(
      InterfaceDeclaration::Member::Type::METHOD, "draw",
      std::move(returnType));
  members.push_back(std::move(member));

  // Create interface declaration with members
  auto interfaceDecl =
      std::make_unique<InterfaceDeclaration>("Drawable", std::move(members));
  auto exportStmt = std::make_unique<ExportStatement>(std::move(interfaceDecl));

  EXPECT_EQ(exportStmt->get_statement().get_node_type(),
            NodeType::INTERFACE_DECLARATION);
}

TEST_F(ImportExportTest, ExportStatementToString) {
  // Create simple variable to export
  auto type = std::make_unique<SimpleType>("bool");
  auto varDecl = std::make_unique<VariableDeclaration>(
      VariableDeclaration::Mutability::LET, "flag", std::move(type));

  auto exportStmt = std::make_unique<ExportStatement>(std::move(varDecl));

  // The to_string should include the inner statement's to_string
  std::string expected = "ExportStatement(VariableDeclaration(let flag))";
  EXPECT_EQ(exportStmt->to_string(), expected);
}

// Combined Import/Export Scenarios
TEST_F(ImportExportTest, MultipleImportsAndExports) {
  auto module = std::make_unique<Module>("library");

  // Add multiple imports
  auto import1 = std::make_unique<ImportStatement>("std.io");
  auto import2 = std::make_unique<ImportStatement>("std.collections", "col");
  auto import3 = std::make_unique<ImportStatement>("utils.math", "math");

  module->add_import(std::move(import1));
  module->add_import(std::move(import2));
  module->add_import(std::move(import3));

  // Add multiple exports
  auto type1 = std::make_unique<SimpleType>("int");
  auto var1 = std::make_unique<VariableDeclaration>(
      VariableDeclaration::Mutability::LET, "VERSION", std::move(type1));
  auto export1 = std::make_unique<ExportStatement>(std::move(var1));

  std::vector<std::unique_ptr<Parameter>> params;
  auto returnType = std::make_unique<SimpleType>("string");
  auto func1 = std::make_unique<FunctionDeclaration>(
      "getName", std::move(params), std::move(returnType));
  auto export2 = std::make_unique<ExportStatement>(std::move(func1));

  module->add_export(std::move(export1));
  module->add_export(std::move(export2));

  // Verify counts
  EXPECT_EQ(module->get_import_count(), 3);
  EXPECT_EQ(module->get_export_count(), 2);

  // Verify import details
  EXPECT_EQ(module->get_imports()[0]->get_module_path(), "std.io");
  EXPECT_FALSE(module->get_imports()[0]->has_alias());

  EXPECT_EQ(module->get_imports()[1]->get_module_path(), "std.collections");
  EXPECT_TRUE(module->get_imports()[1]->has_alias());
  EXPECT_EQ(module->get_imports()[1]->get_alias(), "col");

  EXPECT_EQ(module->get_imports()[2]->get_module_path(), "utils.math");
  EXPECT_EQ(module->get_imports()[2]->get_alias(), "math");
}

// Edge Cases
TEST_F(ImportExportTest, ImportWithSpecialCharacters) {
  auto import = std::make_unique<ImportStatement>("std.io-v2");
  EXPECT_EQ(import->get_module_path(), "std.io-v2");
}

TEST_F(ImportExportTest, ImportWithNumbers) {
  auto import = std::make_unique<ImportStatement>("utils.v2.math3d", "math3d");
  EXPECT_EQ(import->get_module_path(), "utils.v2.math3d");
  EXPECT_EQ(import->get_alias(), "math3d");
}

// Visitor Pattern Tests
TEST_F(ImportExportTest, ImportExportAcceptVisitor) {
  class TestVisitor : public DefaultASTVisitor {
  public:
    int importStatementCount = 0;
    int exportStatementCount = 0;

    void visit(ImportStatement &) override { importStatementCount++; }
    void visit(ExportStatement &) override { exportStatementCount++; }
  };

  TestVisitor visitor;

  // Test ImportStatement
  auto import = std::make_unique<ImportStatement>("test.module");
  import->accept(visitor);

  // Test ExportStatement
  auto type = std::make_unique<SimpleType>("int");
  auto varDecl = std::make_unique<VariableDeclaration>(
      VariableDeclaration::Mutability::LET, "value", std::move(type));
  auto exportStmt = std::make_unique<ExportStatement>(std::move(varDecl));
  exportStmt->accept(visitor);

  EXPECT_EQ(visitor.importStatementCount, 1);
  EXPECT_EQ(visitor.exportStatementCount, 1);
}

// Integration Test
TEST_F(ImportExportTest, FullModuleWithImportsAndExports) {
  auto module = std::make_unique<Module>("calculator");

  // Import math utilities
  auto mathImport = std::make_unique<ImportStatement>("std.math", "math");
  module->add_import(std::move(mathImport));

  // Add a regular statement
  auto type = std::make_unique<SimpleType>("float");
  auto piDecl = std::make_unique<VariableDeclaration>(
      VariableDeclaration::Mutability::LET, "PI", std::move(type));
  module->add_statement(std::move(piDecl));

  // Export a function
  std::vector<std::unique_ptr<Parameter>> params;
  auto param1Type = std::make_unique<SimpleType>("float");
  params.push_back(std::make_unique<Parameter>(
      Parameter::Mutability::LET, "radius", std::move(param1Type)));

  auto returnType = std::make_unique<SimpleType>("float");
  auto circleAreaFunc = std::make_unique<FunctionDeclaration>(
      "circleArea", std::move(params), std::move(returnType));

  auto exportStmt =
      std::make_unique<ExportStatement>(std::move(circleAreaFunc));
  module->add_export(std::move(exportStmt));

  // Verify the complete module structure
  EXPECT_EQ(module->get_module_name(), "calculator");
  EXPECT_EQ(module->get_import_count(), 1);
  EXPECT_EQ(module->get_statement_count(), 1);
  EXPECT_EQ(module->get_export_count(), 1);
  EXPECT_FALSE(module->is_empty());

  std::string expectedString = "Module(calculator, 1 statements)";
  EXPECT_EQ(module->to_string(), expectedString);
}