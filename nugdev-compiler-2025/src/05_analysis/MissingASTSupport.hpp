#pragma once

/**
 * @file MissingASTSupport.hpp
 * @brief EBNF에 정의되었지만 아직 구현되지 않은 AST 클래스들에 대한 지원
 *
 * 이 파일은 현재 04_parsing/ast에 없는 AST 클래스들을 임시로 지원하거나
 * 기존 클래스들로 대체할 수 있는 방안을 제공합니다.
 */

// Forward declarations for missing AST classes
// 실제 프로젝트에서는 적절한 include 경로로 수정 필요

namespace nugdev::compiler::ast {
class ASTNode;
class ASTVisitor;
class Expression;
class Statement;
class Literal;
class StringLiteral;
class Identifier;
class TypeLiteral;
class TernaryExpression;
class FunctionExpression;
class VariableDeclaration;
class FunctionDeclaration;

class DefaultASTVisitor {
public:
  virtual ~DefaultASTVisitor() = default;
  virtual void visit(StringLiteral &node) {}
  virtual void visit(Identifier &node) {}
  // 기타 기본 방문자 메서드들...
};
} // namespace nugdev::compiler::ast

namespace nugdev::compiler::ast {

// =============================================================================
// MISSING LITERAL TYPES - 기존 리터럴로 대체
// =============================================================================

/**
 * @brief CharacterLiteral 대체 - StringLiteral의 특수 경우로 처리
 */
using CharacterLiteral = StringLiteral;

/**
 * @brief NullLiteral 대체 - Identifier로 처리 (예약어 "null")
 */
class NullLiteral : public Literal {
public:
  NullLiteral() : Literal("null") {}

  void accept(ASTVisitor &visitor) override {
    // visitor.visit(*this); // 실제로는 Identifier로 처리
  }
};

/**
 * @brief NoneLiteral 대체 - Identifier로 처리 (예약어 "None")
 */
class NoneLiteral : public Literal {
public:
  NoneLiteral() : Literal("None") {}

  void accept(ASTVisitor &visitor) override {
    // visitor.visit(*this); // 실제로는 Identifier로 처리
  }
};

/**
 * @brief RangeLiteral 대체 - BinaryExpression(..)으로 처리
 */
class RangeLiteral : public Expression {
public:
  std::unique_ptr<Expression> start;
  std::unique_ptr<Expression> end;
  bool is_inclusive = false;
  bool is_infinite = false; // start.. 형태

  RangeLiteral(std::unique_ptr<Expression> start_expr,
               std::unique_ptr<Expression> end_expr = nullptr)
      : start(std::move(start_expr)), end(std::move(end_expr)) {
    is_infinite = (end == nullptr);
  }

  void accept(ASTVisitor &visitor) override {
    // 실제로는 BinaryExpression으로 변환하여 처리
  }
};

// =============================================================================
// MISSING EXPRESSION TYPES - 기존 표현식으로 대체
// =============================================================================

/**
 * @brief IfExpression 대체 - TernaryExpression으로 처리
 */
using IfExpression = TernaryExpression;

/**
 * @brief LambdaExpression 대체 - FunctionExpression으로 처리
 */
using LambdaExpression = FunctionExpression;

/**
 * @brief CastExpression 대체 - PostfixExpression(as/as?)으로 처리
 */
class CastExpression : public Expression {
public:
  std::unique_ptr<Expression> expression;
  std::unique_ptr<TypeLiteral> target_type;
  bool is_safe_cast = false; // as? vs as

  CastExpression(std::unique_ptr<Expression> expr,
                 std::unique_ptr<TypeLiteral> type, bool safe = false)
      : expression(std::move(expr)), target_type(std::move(type)),
        is_safe_cast(safe) {}

  void accept(ASTVisitor &visitor) override {
    // 실제로는 PostfixExpression으로 변환하여 처리
  }
};

// =============================================================================
// MISSING TYPE SYSTEM CLASSES
// =============================================================================

/**
 * @brief FunctionType 대체 - TypeLiteral의 확장
 */
class FunctionType : public TypeLiteral {
public:
  std::vector<std::unique_ptr<TypeLiteral>> parameter_types;
  std::unique_ptr<TypeLiteral> return_type;

  FunctionType(std::vector<std::unique_ptr<TypeLiteral>> params,
               std::unique_ptr<TypeLiteral> ret_type)
      : TypeLiteral("function"), parameter_types(std::move(params)),
        return_type(std::move(ret_type)) {}

  void accept(ASTVisitor &visitor) override {
    // 기본 TypeLiteral 방문자 사용
  }
};

/**
 * @brief OptionalType 대체 - TypeLiteral + "?" 표시
 */
class OptionalType : public TypeLiteral {
public:
  std::unique_ptr<TypeLiteral> inner_type;

  explicit OptionalType(std::unique_ptr<TypeLiteral> inner)
      : TypeLiteral(inner->get_name() + "?"), inner_type(std::move(inner)) {}

  void accept(ASTVisitor &visitor) override {
    // 기본 TypeLiteral 방문자 사용
  }
};

/**
 * @brief TupleType 대체 - TypeLiteral의 확장
 */
class TupleType : public TypeLiteral {
public:
  std::vector<std::unique_ptr<TypeLiteral>> element_types;

  explicit TupleType(std::vector<std::unique_ptr<TypeLiteral>> elements)
      : TypeLiteral("tuple"), element_types(std::move(elements)) {}

  void accept(ASTVisitor &visitor) override {
    // 기본 TypeLiteral 방문자 사용
  }
};

// =============================================================================
// MISSING WHEN EXPRESSION SUPPORT
// =============================================================================

/**
 * @brief WhenCondition 대체 클래스들
 */
class WhenCondition : public ASTNode {
public:
  virtual ~WhenCondition() = default;
  virtual bool matches(const Expression &value) const = 0;
};

class ValueCondition : public WhenCondition {
public:
  std::unique_ptr<Expression> value;

  explicit ValueCondition(std::unique_ptr<Expression> val)
      : value(std::move(val)) {}

  bool matches(const Expression &test_value) const override {
    // 실제 구현 필요
    return false;
  }

  void accept(ASTVisitor &visitor) override {}
};

class RangeCondition : public WhenCondition {
public:
  std::unique_ptr<Expression> variable;
  std::unique_ptr<RangeLiteral> range;

  RangeCondition(std::unique_ptr<Expression> var,
                 std::unique_ptr<RangeLiteral> rng)
      : variable(std::move(var)), range(std::move(rng)) {}

  bool matches(const Expression &test_value) const override {
    // 실제 구현 필요
    return false;
  }

  void accept(ASTVisitor &visitor) override {}
};

class TypeCondition : public WhenCondition {
public:
  std::unique_ptr<Expression> expression;
  std::unique_ptr<TypeLiteral> type;

  TypeCondition(std::unique_ptr<Expression> expr,
                std::unique_ptr<TypeLiteral> type_lit)
      : expression(std::move(expr)), type(std::move(type_lit)) {}

  bool matches(const Expression &test_value) const override {
    // 실제 구현 필요
    return false;
  }

  void accept(ASTVisitor &visitor) override {}
};

class GuardCondition : public WhenCondition {
public:
  std::unique_ptr<WhenCondition> base_condition;
  std::unique_ptr<Expression> guard_expression;

  GuardCondition(std::unique_ptr<WhenCondition> base,
                 std::unique_ptr<Expression> guard)
      : base_condition(std::move(base)), guard_expression(std::move(guard)) {}

  bool matches(const Expression &test_value) const override {
    // 실제 구현 필요
    return false;
  }

  void accept(ASTVisitor &visitor) override {}
};

class MultipleCondition : public WhenCondition {
public:
  std::vector<std::unique_ptr<WhenCondition>> conditions;

  explicit MultipleCondition(std::vector<std::unique_ptr<WhenCondition>> conds)
      : conditions(std::move(conds)) {}

  bool matches(const Expression &test_value) const override {
    // 실제 구현 필요 - OR 로직
    return false;
  }

  void accept(ASTVisitor &visitor) override {}
};

// =============================================================================
// MISSING IMPORT/EXPORT STATEMENTS
// =============================================================================

/**
 * @brief ImportStatement 대체 - Statement의 확장
 */
class ImportStatement : public Statement {
public:
  std::unique_ptr<StringLiteral> module_path;
  std::unique_ptr<Identifier> alias;

  ImportStatement(std::unique_ptr<StringLiteral> path,
                  std::unique_ptr<Identifier> as_alias = nullptr)
      : module_path(std::move(path)), alias(std::move(as_alias)) {}

  void accept(ASTVisitor &visitor) override {
    // 기본 Statement 방문자 사용
  }
};

/**
 * @brief ExportStatement 대체 - Statement의 확장
 */
class ExportStatement : public Statement {
public:
  std::unique_ptr<Statement> exported_statement;

  explicit ExportStatement(std::unique_ptr<Statement> stmt)
      : exported_statement(std::move(stmt)) {}

  void accept(ASTVisitor &visitor) override {
    // 기본 Statement 방문자 사용
  }
};

// =============================================================================
// MISSING DECLARATION TYPES
// =============================================================================

/**
 * @brief StructDeclaration 대체 - Declaration 기반
 */
class StructDeclaration : public Statement {
public:
  std::unique_ptr<Identifier> name;
  std::vector<std::unique_ptr<VariableDeclaration>> fields;

  StructDeclaration(
      std::unique_ptr<Identifier> struct_name,
      std::vector<std::unique_ptr<VariableDeclaration>> field_list)
      : name(std::move(struct_name)), fields(std::move(field_list)) {}

  void accept(ASTVisitor &visitor) override {
    // 기본 Statement 방문자 사용
  }
};

/**
 * @brief InterfaceDeclaration 대체 - Declaration 기반
 */
class InterfaceDeclaration : public Statement {
public:
  std::unique_ptr<Identifier> name;
  std::vector<std::unique_ptr<FunctionDeclaration>> methods;

  InterfaceDeclaration(
      std::unique_ptr<Identifier> interface_name,
      std::vector<std::unique_ptr<FunctionDeclaration>> method_list)
      : name(std::move(interface_name)), methods(std::move(method_list)) {}

  void accept(ASTVisitor &visitor) override {
    // 기본 Statement 방문자 사용
  }
};

// =============================================================================
// AST 클래스 존재 확인 유틸리티
// =============================================================================

/**
 * @brief AST 클래스 사용 가능성 검사
 */
class ASTClassChecker {
public:
  // 각 클래스가 실제로 사용 가능한지 확인
  static bool has_character_literal() { return true; } // 대체 가능
  static bool has_null_literal() { return true; }      // 대체 가능
  static bool has_none_literal() { return true; }      // 대체 가능
  static bool has_range_literal() { return true; }     // 대체 가능
  static bool has_if_expression() { return true; } // TernaryExpression으로 대체
  static bool has_lambda_expression() {
    return true;
  } // FunctionExpression으로 대체
  static bool has_cast_expression() {
    return true;
  } // PostfixExpression으로 대체
  static bool has_when_expressions() { return true; }       // 대체 구현 제공
  static bool has_struct_declarations() { return true; }    // 대체 구현 제공
  static bool has_interface_declarations() { return true; } // 대체 구현 제공

  // 완전히 구현되지 않은 기능들
  static bool has_full_ebnf_support() { return false; }
  static bool has_template_system() { return false; }
  static bool has_async_await() { return false; }

  // 권장 대체 방안 제공
  static std::string
  get_replacement_suggestion(const std::string &missing_class);
};

} // namespace nugdev::compiler::ast

// =============================================================================
// VISITOR PATTERN 확장 - 누락된 클래스들을 위한 기본 구현
// =============================================================================

namespace nugdev::compiler::analysis {

/**
 * @brief 누락된 AST 클래스들을 처리하는 확장 방문자
 */
class ExtendedASTVisitor : public ast::DefaultASTVisitor {
public:
  // 대체 클래스들에 대한 기본 방문자 구현
  virtual void visit(ast::CharacterLiteral &node) {
    // StringLiteral로 위임
    auto &str_literal = static_cast<ast::StringLiteral &>(node);
    visit(str_literal);
  }

  virtual void visit(ast::NullLiteral &node) {
    // Identifier("null")로 처리
  }

  virtual void visit(ast::NoneLiteral &node) {
    // Identifier("None")로 처리
  }

  virtual void visit(ast::RangeLiteral &node) {
    // BinaryExpression(..)로 처리
    if (node.start)
      node.start->accept(*this);
    if (node.end)
      node.end->accept(*this);
  }

  virtual void visit(ast::CastExpression &node) {
    // PostfixExpression(as/as?)로 처리
    if (node.expression)
      node.expression->accept(*this);
    if (node.target_type)
      node.target_type->accept(*this);
  }

  virtual void visit(ast::StructDeclaration &node) {
    if (node.name)
      node.name->accept(*this);
    for (auto &field : node.fields) {
      if (field)
        field->accept(*this);
    }
  }

  virtual void visit(ast::InterfaceDeclaration &node) {
    if (node.name)
      node.name->accept(*this);
    for (auto &method : node.methods) {
      if (method)
        method->accept(*this);
    }
  }
};

} // namespace nugdev::compiler::analysis