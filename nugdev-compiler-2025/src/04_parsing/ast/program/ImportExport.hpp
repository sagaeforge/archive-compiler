#pragma once

#include <04_parsing/ast/core/ASTNode.hpp>
#include <04_parsing/ast/statements/Statements.hpp>
#include <memory>
#include <string>

namespace nugdev {
namespace ast {

// Forward declarations
class ASTVisitor;
class Statement;

/**
 * @brief Import statement for importing modules or symbols
 *
 * EBNF: import_statement = "import" string_literal [ "as" identifier ] [ ";" ]
 * ;
 */
class ImportStatement : public ASTNode {
public:
  explicit ImportStatement(const std::string &modulePath,
                           const std::string &alias = "")
      : ASTNode(NodeType::IMPORT_STATEMENT), modulePath(modulePath),
        alias(alias) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    std::string result = "ImportStatement(\"" + modulePath + "\"";
    if (!alias.empty()) {
      result += " as " + alias;
    }
    result += ")";
    return result;
  }

  const std::string &get_module_path() const { return modulePath; }

  bool has_alias() const { return !alias.empty(); }
  const std::string &get_alias() const { return alias; }

private:
  std::string modulePath; // Path to the module to import
  std::string alias;      // Optional alias name
};

/**
 * @brief Export statement for exporting statements or symbols
 *
 * EBNF: export_statement = "export" statement ;
 */
class ExportStatement : public ASTNode {
public:
  explicit ExportStatement(std::unique_ptr<Statement> statement)
      : ASTNode(NodeType::EXPORT_STATEMENT), statement(std::move(statement)) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    return "ExportStatement(" + statement->to_string() + ")";
  }

  const Statement &get_statement() const { return *statement; }

private:
  std::unique_ptr<Statement> statement; // Statement to export
};

} // namespace ast
} // namespace nugdev