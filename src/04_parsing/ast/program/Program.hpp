#pragma once

#include <04_parsing/ast/core/ASTNode.hpp>
#include <memory>
#include <string>
#include <vector>

namespace nugdev {
namespace ast {

// Forward declarations
class ASTVisitor;
class Module;
class Statement;

/**
 * @brief Root AST node representing a complete program
 *
 * A program consists of one or more modules, where each module
 * contains a collection of top-level statements.
 */
class Program : public ASTNode {
public:
  explicit Program() : ASTNode(NodeType::PROGRAM) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    return "Program(" + std::to_string(modules.size()) + " modules)";
  }

  // Module management
  void add_module(std::unique_ptr<Module> module) {
    modules.push_back(std::move(module));
  }

  const std::vector<std::unique_ptr<Module>> &get_modules() const {
    return modules;
  }

  size_t get_module_count() const { return modules.size(); }

  bool is_empty() const { return modules.empty(); }

private:
  std::vector<std::unique_ptr<Module>> modules;
};

// Forward declarations for statements
class ImportStatement;
class ExportStatement;

/**
 * @brief Module (collection of statements)
 *
 * A module represents a single compilation unit with a name
 * and a collection of top-level statements.
 */
class Module : public ASTNode {
public:
  explicit Module(const std::string &name = "")
      : ASTNode(NodeType::MODULE), moduleName(name) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    return "Module(" + moduleName + ", " + std::to_string(statements.size()) +
           " statements)";
  }

  const std::string &get_module_name() const { return moduleName; }

  void add_statement(std::unique_ptr<Statement> statement) {
    statements.push_back(std::move(statement));
  }

  void add_import(std::unique_ptr<ImportStatement> import) {
    imports.push_back(std::move(import));
  }

  void add_export(std::unique_ptr<ExportStatement> export_stmt) {
    exports.push_back(std::move(export_stmt));
  }

  const std::vector<std::unique_ptr<Statement>> &get_statements() const {
    return statements;
  }

  const std::vector<std::unique_ptr<ImportStatement>> &get_imports() const {
    return imports;
  }

  const std::vector<std::unique_ptr<ExportStatement>> &get_exports() const {
    return exports;
  }

  size_t get_statement_count() const { return statements.size(); }
  size_t get_import_count() const { return imports.size(); }
  size_t get_export_count() const { return exports.size(); }

  bool is_empty() const {
    return statements.empty() && imports.empty() && exports.empty();
  }

private:
  std::string moduleName;
  std::vector<std::unique_ptr<Statement>> statements;
  std::vector<std::unique_ptr<ImportStatement>> imports;
  std::vector<std::unique_ptr<ExportStatement>> exports;
};

} // namespace ast
} // namespace nugdev