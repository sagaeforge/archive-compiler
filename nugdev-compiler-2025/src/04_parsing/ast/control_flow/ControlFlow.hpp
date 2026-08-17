#pragma once

#include <04_parsing/ast/core/ASTNode.hpp>
#include <04_parsing/ast/expressions/Expressions.hpp>
#include <04_parsing/ast/statements/Statements.hpp>
#include <memory>
#include <string>
#include <vector>

namespace nugdev {
namespace ast {

// Forward declarations
class ASTVisitor;
class Expression;
class Statement;
class BlockExpression;

/**
 * @brief If statement with optional elif and else clauses
 *
 * EBNF: if_statement = [ label ] "if" [ "(" expression ")" ] "{" {
 * statement_or_expression } "}" { "elif" [ "(" expression ")" ] "{" {
 * statement_or_expression } "}" } [ "else" "{" { statement_or_expression } "}"
 * ] ;
 */
class IfStatement : public Statement {
public:
  struct IfClause {
    std::unique_ptr<Expression> condition; // nullptr for unconditional clause
    std::unique_ptr<BlockExpression> body;

    IfClause(std::unique_ptr<Expression> cond,
             std::unique_ptr<BlockExpression> body)
        : condition(std::move(cond)), body(std::move(body)) {}
  };

  explicit IfStatement() : Statement(NodeType::IF_STATEMENT) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    return "IfStatement(" + std::to_string(branches.size()) + " branches)";
  }

  // Condition and block management
  struct Branch {
    std::unique_ptr<Expression> condition; // null for else branch
    std::unique_ptr<BlockExpression> block;

    Branch(std::unique_ptr<Expression> cond,
           std::unique_ptr<BlockExpression> bl)
        : condition(std::move(cond)), block(std::move(bl)) {}
  };

  void add_if_branch(std::unique_ptr<Expression> condition,
                     std::unique_ptr<BlockExpression> block) {
    branches.emplace_back(std::move(condition), std::move(block));
  }

  void add_else_if_branch(std::unique_ptr<Expression> condition,
                          std::unique_ptr<BlockExpression> block) {
    branches.emplace_back(std::move(condition), std::move(block));
  }

  void add_else_branch(std::unique_ptr<BlockExpression> block) {
    branches.emplace_back(nullptr, std::move(block)); // null condition = else
  }

  const std::vector<Branch> &get_branches() const { return branches; }

  size_t get_branch_count() const { return branches.size(); }

  bool has_else_branch() const {
    return !branches.empty() && branches.back().condition == nullptr;
  }

private:
  std::vector<Branch> branches;
};

/**
 * @brief For loop statement with various clause types
 *
 * EBNF: for_statement = [ label ] "for" for_clause "{" {
 * statement_or_expression } "}" ; for_clause = (* empty *) (* infinite loop *)
 *            | "(" expression ")" (* while-style *) | "(" variable_declaration
 * ";" expression ")"                    (* init; condition *) | "("
 * variable_declaration ";" expression ";" expression ")"     (* init;
 * condition; increment *) | "(" identifier "in" expression ")" (* for-in loop
 * *) ;
 */
class ForStatement : public Statement {
public:
  enum class ForType {
    INFINITE, // for { ... }
    WHILE,    // for (condition) { ... }
    C_STYLE,  // for (init; condition; increment) { ... }
    FOR_IN    // for (item in collection) { ... }
  };

  explicit ForStatement(ForType type)
      : Statement(NodeType::FOR_STATEMENT), forType(type) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    return "ForStatement(" + get_for_type_string() + ")";
  }

  ForType get_for_type() const { return forType; }

  std::string get_for_type_string() const {
    switch (forType) {
    case ForType::INFINITE:
      return "infinite";
    case ForType::WHILE:
      return "while";
    case ForType::C_STYLE:
      return "c-style";
    case ForType::FOR_IN:
      return "for-in";
    default:
      return "unknown";
    }
  }

  // Loop body
  void set_body(std::unique_ptr<BlockExpression> body) {
    this->body = std::move(body);
  }

  const BlockExpression &get_body() const { return *body; }

  // For while-style loops
  void set_condition(std::unique_ptr<Expression> condition) {
    this->condition = std::move(condition);
  }

  const Expression *get_condition() const { return condition.get(); }

  // For C-style loops
  void set_init_statement(std::unique_ptr<Statement> init) {
    initStatement = std::move(init);
  }

  void set_increment_expression(std::unique_ptr<Expression> increment) {
    incrementExpression = std::move(increment);
  }

  const Statement *get_init_statement() const { return initStatement.get(); }
  const Expression *get_increment_expression() const {
    return incrementExpression.get();
  }

  // For for-in loops
  void set_iterator_variable(const std::string &variable) {
    iteratorVariable = variable;
  }

  void set_iterable_expression(std::unique_ptr<Expression> iterable) {
    iterableExpression = std::move(iterable);
  }

  const std::string &get_iterator_variable() const { return iteratorVariable; }
  const Expression *get_iterable_expression() const {
    return iterableExpression.get();
  }

private:
  ForType forType;
  std::unique_ptr<BlockExpression> body;

  // For while and C-style loops
  std::unique_ptr<Expression> condition;

  // For C-style loops only
  std::unique_ptr<Statement> initStatement;
  std::unique_ptr<Expression> incrementExpression;

  // For for-in loops only
  std::string iteratorVariable;
  std::unique_ptr<Expression> iterableExpression;
};

/**
 * @brief Break statement for exiting loops or labeled blocks
 *
 * EBNF: break_statement = "break" [ "@" identifier ] [ ";" ] ;
 */
class BreakStatement : public Statement {
public:
  explicit BreakStatement(const std::string &targetLabel = "")
      : Statement(NodeType::BREAK_STATEMENT), targetLabel(targetLabel) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    if (targetLabel.empty()) {
      return "BreakStatement()";
    } else {
      return "BreakStatement(@" + targetLabel + ")";
    }
  }

  const std::string &get_target_label() const { return targetLabel; }
  bool has_target_label() const { return !targetLabel.empty(); }

private:
  std::string targetLabel; // Optional target label
};

/**
 * @brief Continue statement for skipping to next loop iteration
 *
 * EBNF: continue_statement = "continue" [ "@" identifier ] [ ";" ] ;
 */
class ContinueStatement : public Statement {
public:
  explicit ContinueStatement(const std::string &targetLabel = "")
      : Statement(NodeType::CONTINUE_STATEMENT), targetLabel(targetLabel) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    if (targetLabel.empty()) {
      return "ContinueStatement()";
    } else {
      return "ContinueStatement(@" + targetLabel + ")";
    }
  }

  const std::string &get_target_label() const { return targetLabel; }
  bool has_target_label() const { return !targetLabel.empty(); }

private:
  std::string targetLabel; // Optional target label
};

/**
 * @brief Return statement for exiting functions
 *
 * EBNF: return_statement = "return" [ "@" identifier ] [ expression ] [ ";" ] ;
 */
class ReturnStatement : public Statement {
public:
  explicit ReturnStatement(std::unique_ptr<Expression> returnValue = nullptr,
                           const std::string &targetLabel = "")
      : Statement(NodeType::RETURN_STATEMENT),
        returnValue(std::move(returnValue)), targetLabel(targetLabel) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    std::string result = "ReturnStatement(";
    if (!targetLabel.empty()) {
      result += "@" + targetLabel;
      if (returnValue)
        result += ", ";
    }
    if (returnValue) {
      result += "with value";
    }
    result += ")";
    return result;
  }

  bool has_return_value() const { return returnValue != nullptr; }
  const Expression *get_return_value() const { return returnValue.get(); }

  const std::string &get_target_label() const { return targetLabel; }
  bool has_target_label() const { return !targetLabel.empty(); }

private:
  std::unique_ptr<Expression> returnValue; // Optional return value
  std::string targetLabel;                 // Optional target label
};

} // namespace ast
} // namespace nugdev