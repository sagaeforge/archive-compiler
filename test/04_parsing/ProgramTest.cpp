#include <04_parsing/ast/core/AST.hpp>
#include <gtest/gtest.h>

using namespace nugdev::ast;

class ProgramTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Setup code if needed
  }
};

// Program Tests
TEST_F(ProgramTest, ProgramBasic) {
  auto program = std::make_unique<Program>();

  ASSERT_NE(program, nullptr);
  EXPECT_EQ(program->get_node_type(), NodeType::PROGRAM);
  EXPECT_EQ(program->get_modules().size(), 0);
  EXPECT_TRUE(program->is_empty());
  EXPECT_EQ(program->to_string(), "Program(0 modules)");
}

TEST_F(ProgramTest, ProgramWithModules) {
  auto program = std::make_unique<Program>();

  // Add first module
  auto module1 = std::make_unique<Module>("main");
  program->add_module(std::move(module1));

  // Add second module
  auto module2 = std::make_unique<Module>("utils");
  program->add_module(std::move(module2));

  EXPECT_EQ(program->get_modules().size(), 2);
  EXPECT_EQ(program->get_module_count(), 2);
  EXPECT_FALSE(program->is_empty());
  EXPECT_EQ(program->to_string(), "Program(2 modules)");
}

// Module Tests
TEST_F(ProgramTest, ModuleBasic) {
  auto module = std::make_unique<Module>("test_module");

  ASSERT_NE(module, nullptr);
  EXPECT_EQ(module->get_node_type(), NodeType::MODULE);
  EXPECT_EQ(module->get_module_name(), "test_module");
  EXPECT_EQ(module->get_statements().size(), 0);
  EXPECT_EQ(module->get_imports().size(), 0);
  EXPECT_EQ(module->get_exports().size(), 0);
  EXPECT_TRUE(module->is_empty());
}

TEST_F(ProgramTest, ModuleWithStatements) {
  auto module = std::make_unique<Module>("main");

  // Add variable declaration
  auto type = std::make_unique<SimpleType>("int");
  auto varDecl = std::make_unique<VariableDeclaration>(
      VariableDeclaration::Mutability::LET, "x", std::move(type));
  module->add_statement(std::move(varDecl));

  // Add function declaration
  std::vector<std::unique_ptr<Parameter>> params;
  auto returnType = std::make_unique<SimpleType>("void");
  auto funcDecl = std::make_unique<FunctionDeclaration>(
      "testFunc", std::move(params), std::move(returnType));
  module->add_statement(std::move(funcDecl));

  EXPECT_EQ(module->get_statements().size(), 2);
  EXPECT_EQ(module->get_statement_count(), 2);
  EXPECT_FALSE(module->is_empty());
}

TEST_F(ProgramTest, ModuleWithImports) {
  auto module = std::make_unique<Module>("main");

  // Add imports
  auto import1 = std::make_unique<ImportStatement>("std.io");
  auto import2 = std::make_unique<ImportStatement>("utils.math", "math");

  module->add_import(std::move(import1));
  module->add_import(std::move(import2));

  EXPECT_EQ(module->get_imports().size(), 2);
  EXPECT_EQ(module->get_import_count(), 2);
  EXPECT_EQ(module->get_imports()[0]->get_module_path(), "std.io");
  EXPECT_EQ(module->get_imports()[1]->get_module_path(), "utils.math");
  EXPECT_EQ(module->get_imports()[1]->get_alias(), "math");
}

TEST_F(ProgramTest, ModuleWithExports) {
  auto module = std::make_unique<Module>("library");

  // Create a function to export
  std::vector<std::unique_ptr<Parameter>> params;
  auto returnType = std::make_unique<SimpleType>("int");
  auto funcDecl = std::make_unique<FunctionDeclaration>(
      "publicFunc", std::move(params), std::move(returnType));

  auto exportStmt = std::make_unique<ExportStatement>(std::move(funcDecl));
  module->add_export(std::move(exportStmt));

  EXPECT_EQ(module->get_exports().size(), 1);
  EXPECT_EQ(module->get_export_count(), 1);
}

TEST_F(ProgramTest, ModuleToString) {
  auto module = std::make_unique<Module>("testModule");

  // Add one statement
  auto type = std::make_unique<SimpleType>("string");
  auto varDecl = std::make_unique<VariableDeclaration>(
      VariableDeclaration::Mutability::LET, "name", std::move(type));
  module->add_statement(std::move(varDecl));

  EXPECT_EQ(module->to_string(), "Module(testModule, 1 statements)");
}

// Complex Program Structure Test
TEST_F(ProgramTest, ComplexProgramStructure) {
  auto program = std::make_unique<Program>();

  // Create main module
  auto mainModule = std::make_unique<Module>("main");

  // Add import to main module
  auto import = std::make_unique<ImportStatement>("std.collections", "col");
  mainModule->add_import(std::move(import));

  // Add variable declaration to main module
  auto type = std::make_unique<SimpleType>("int");
  auto varDecl = std::make_unique<VariableDeclaration>(
      VariableDeclaration::Mutability::LET, "count", std::move(type));
  mainModule->add_statement(std::move(varDecl));

  program->add_module(std::move(mainModule));

  // Create utils module
  auto utilsModule = std::make_unique<Module>("utils");

  // Add function to utils module
  std::vector<std::unique_ptr<Parameter>> params;
  auto paramType = std::make_unique<SimpleType>("int");
  params.push_back(std::make_unique<Parameter>(Parameter::Mutability::LET, "n",
                                               std::move(paramType)));

  auto returnType = std::make_unique<SimpleType>("int");
  auto funcDecl = std::make_unique<FunctionDeclaration>(
      "square", std::move(params), std::move(returnType));

  // Export the function
  auto exportStmt = std::make_unique<ExportStatement>(std::move(funcDecl));
  utilsModule->add_export(std::move(exportStmt));

  program->add_module(std::move(utilsModule));

  // Verify the complex structure
  EXPECT_EQ(program->get_modules().size(), 2);
  EXPECT_EQ(program->get_modules()[0]->get_module_name(), "main");
  EXPECT_EQ(program->get_modules()[1]->get_module_name(), "utils");

  EXPECT_EQ(program->get_modules()[0]->get_import_count(), 1);
  EXPECT_EQ(program->get_modules()[0]->get_statement_count(), 1);

  EXPECT_EQ(program->get_modules()[1]->get_export_count(), 1);
}

// Visitor Pattern Tests
TEST_F(ProgramTest, ProgramModuleAcceptVisitor) {
  class TestVisitor : public DefaultASTVisitor {
  public:
    int programCount = 0;
    int moduleCount = 0;

    void visit(Program &) override { programCount++; }
    void visit(Module &) override { moduleCount++; }
  };

  TestVisitor visitor;

  // Test Program
  auto program = std::make_unique<Program>();
  program->accept(visitor);

  // Test Module
  auto module = std::make_unique<Module>("test");
  module->accept(visitor);

  EXPECT_EQ(visitor.programCount, 1);
  EXPECT_EQ(visitor.moduleCount, 1);
}