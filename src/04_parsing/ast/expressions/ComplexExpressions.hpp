#pragma once

#include <04_parsing/ast/core/ASTNode.hpp>
#include <04_parsing/ast/expressions/Expressions.hpp>
#include <04_parsing/ast/statements/Statements.hpp>
#include <04_parsing/ast/types/Types.hpp>
#include <memory>
#include <string>
#include <vector>

namespace nugdev {
namespace ast {

// Forward declarations
class ASTVisitor;
class Expression;
class Statement;
class TypeLiteral;
class Parameter;

/**
 * @brief Block expression that can contain statements and an optional final
 * expression
 *
 * EBNF: block_expression = "{" { statement } [ expression ] "}" ;
 */
class BlockExpression : public Expression {
public:
  explicit BlockExpression() : Expression(NodeType::BLOCK_EXPRESSION) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    return "BlockExpression(" + std::to_string(statements.size()) +
           " statements)";
  }

  std::string get_expression_type() const override { return "block"; }

  // Statement management
  void add_statement(std::unique_ptr<Statement> statement) {
    statements.push_back(std::move(statement));
  }

  const std::vector<std::unique_ptr<Statement>> &get_statements() const {
    return statements;
  }

  size_t get_statement_count() const { return statements.size(); }

  bool is_empty() const { return statements.empty(); }

  // The result expression (optional)
  void set_result_expression(std::unique_ptr<Expression> expr) {
    resultExpression = std::move(expr);
  }

  const Expression *get_result_expression() const {
    return resultExpression.get();
  }

  bool has_result_expression() const { return resultExpression != nullptr; }

private:
  std::vector<std::unique_ptr<Statement>> statements;
  std::unique_ptr<Expression> resultExpression; // Optional final expression
};

/**
 * @brief If expression (ternary-like but with blocks)
 *
 * EBNF: if_expression = "if" expression block_expression [ "else"
 * ( if_expression | block_expression ) ] ;
 */
class IfExpression : public Expression {
public:
  explicit IfExpression(std::unique_ptr<Expression> condition,
                        std::unique_ptr<BlockExpression> thenBlock)
      : Expression(NodeType::IF_EXPRESSION), condition(std::move(condition)),
        thenBlock(std::move(thenBlock)) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override { return "IfExpression"; }

  std::string get_expression_type() const override { return "if"; }

  const Expression &get_condition() const { return *condition; }
  const BlockExpression &get_then_block() const { return *thenBlock; }

  // Else clause (can be another IfExpression or BlockExpression)
  void set_else_block(std::unique_ptr<Expression> elseExpr) {
    elseBlock = std::move(elseExpr);
  }

  const Expression *get_else_block() const { return elseBlock.get(); }
  bool has_else_block() const { return elseBlock != nullptr; }

private:
  std::unique_ptr<Expression> condition;
  std::unique_ptr<BlockExpression> thenBlock;
  std::unique_ptr<Expression>
      elseBlock; // Can be IfExpression or BlockExpression
};

// Forward declarations for when conditions
class WhenCondition;

/**
 * @brief When expression (pattern matching)
 *
 * EBNF: when_expression = "when" expression "{" { when_branch } [ else_branch ]
 * "}" ;
 */
class WhenExpression : public Expression {
public:
  explicit WhenExpression(std::unique_ptr<Expression> scrutinee)
      : Expression(NodeType::WHEN_EXPRESSION), scrutinee(std::move(scrutinee)) {
  }

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    return "WhenExpression(" + std::to_string(branches.size()) + " branches)";
  }

  std::string get_expression_type() const override { return "when"; }

  const Expression &get_scrutinee() const { return *scrutinee; }

  // When branches
  struct WhenBranch {
    std::unique_ptr<WhenCondition> condition;
    std::unique_ptr<Expression> expression;

    WhenBranch(std::unique_ptr<WhenCondition> cond,
               std::unique_ptr<Expression> expr)
        : condition(std::move(cond)), expression(std::move(expr)) {}
  };

  void add_branch(std::unique_ptr<WhenCondition> condition,
                  std::unique_ptr<Expression> expression) {
    branches.emplace_back(std::move(condition), std::move(expression));
  }

  const std::vector<WhenBranch> &get_branches() const { return branches; }

  // Else branch (optional)
  void set_else_branch(std::unique_ptr<Expression> elseExpr) {
    elseBranch = std::move(elseExpr);
  }

  const Expression *get_else_branch() const { return elseBranch.get(); }
  bool has_else_branch() const { return elseBranch != nullptr; }

private:
  std::unique_ptr<Expression> scrutinee;
  std::vector<WhenBranch> branches;
  std::unique_ptr<Expression> elseBranch; // Optional else branch
};

/**
 * @brief Function expression (anonymous function)
 *
 * EBNF: function_expression = [ label ] "fun" "(" [ parameter_list ] ")" ":"
 * type_literal function_expression_body ;
 */
class FunctionExpression : public Expression {
public:
  enum class BodyType {
    BLOCK,     // { statements }
    EXPRESSION // = expression
  };

  explicit FunctionExpression(
      std::vector<std::unique_ptr<Parameter>> parameters,
      std::unique_ptr<TypeLiteral> returnType)
      : Expression(NodeType::FUNCTION_EXPRESSION),
        parameters(std::move(parameters)), returnType(std::move(returnType)),
        bodyType(BodyType::BLOCK) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    return "FunctionExpression(" + std::to_string(parameters.size()) +
           " params)";
  }

  std::string get_expression_type() const override { return "function"; }

  const std::vector<std::unique_ptr<Parameter>> &get_parameters() const {
    return parameters;
  }

  const TypeLiteral &get_return_type() const { return *returnType; }

  BodyType get_body_type() const { return bodyType; }

  // For block body
  void set_block_body(std::unique_ptr<BlockExpression> block) {
    blockBody = std::move(block);
    bodyType = BodyType::BLOCK;
  }

  const BlockExpression *get_block_body() const { return blockBody.get(); }

  // For expression body
  void set_expression_body(std::unique_ptr<Expression> expr) {
    expressionBody = std::move(expr);
    bodyType = BodyType::EXPRESSION;
  }

  const Expression *get_expression_body() const { return expressionBody.get(); }

private:
  std::vector<std::unique_ptr<Parameter>> parameters;
  std::unique_ptr<TypeLiteral> returnType;

  BodyType bodyType;
  std::unique_ptr<BlockExpression> blockBody; // For block body
  std::unique_ptr<Expression> expressionBody; // For expression body
};

/**
 * @brief Lambda expression (anonymous function)
 *
 * EBNF: lambda_expression = "|" [ parameter_list ] "|" ( "->" type_literal )?
 * ( expression | block_expression ) ;
 */
class LambdaExpression : public Expression {
public:
  explicit LambdaExpression(std::vector<std::unique_ptr<Parameter>> parameters)
      : Expression(NodeType::LAMBDA_EXPRESSION),
        parameters(std::move(parameters)) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    return "LambdaExpression(" + std::to_string(parameters.size()) +
           " parameters)";
  }

  std::string get_expression_type() const override { return "lambda"; }

  const std::vector<std::unique_ptr<Parameter>> &get_parameters() const {
    return parameters;
  }

  // Return type annotation (optional)
  void set_return_type(std::unique_ptr<TypeLiteral> type) {
    returnType = std::move(type);
  }

  const TypeLiteral *get_return_type() const { return returnType.get(); }
  bool has_return_type() const { return returnType != nullptr; }

  // Body can be either expression or block
  void set_expression_body(std::unique_ptr<Expression> expr) {
    body = std::move(expr);
    isBlockBody = false;
  }

  void set_block_body(std::unique_ptr<BlockExpression> block) {
    body = std::move(block);
    isBlockBody = true;
  }

  const Expression *get_body() const { return body.get(); }
  bool is_block_body() const { return isBlockBody; }

private:
  std::vector<std::unique_ptr<Parameter>> parameters;
  std::unique_ptr<TypeLiteral> returnType; // Optional return type
  std::unique_ptr<Expression> body;        // Expression or BlockExpression
  bool isBlockBody = false;
};

} // namespace ast
} // namespace nugdev