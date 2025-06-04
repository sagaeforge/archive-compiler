#pragma once

#include <04_parsing/ast/core/ASTNode.hpp>
#include <04_parsing/ast/expressions/Expressions.hpp>
#include <04_parsing/ast/types/Types.hpp>
#include <memory>
#include <string>
#include <vector>

namespace nugdev {
namespace ast {

// Forward declarations
class ASTVisitor;
class Expression;
class TypeLiteral;

/**
 * @brief Base class for when conditions in when expressions
 */
class WhenCondition : public ASTNode {
public:
  explicit WhenCondition(NodeType type) : ASTNode(type) {}

  virtual ~WhenCondition() = default;

  void accept(ASTVisitor &visitor) override = 0;
  void accept(ASTVisitor &visitor) const override = 0;

  std::string to_string() const override { return "WhenCondition"; }

  virtual bool matches(const Expression &value) const = 0;
};

/**
 * @brief Value condition for when expressions (direct value matching)
 *
 * EBNF: value_condition = expression ;
 */
class ValueCondition : public WhenCondition {
public:
  explicit ValueCondition(std::unique_ptr<Expression> value)
      : WhenCondition(NodeType::VALUE_CONDITION), value(std::move(value)) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    return "ValueCondition(" + value->to_string() + ")";
  }

  const Expression &get_value() const { return *value; }

  bool matches(const Expression &testValue) const override {
    // Implementation would compare expressions for equality
    return value->to_string() == testValue.to_string();
  }

private:
  std::unique_ptr<Expression> value;
};

/**
 * @brief Range condition for when expressions (in operator)
 *
 * EBNF: range_condition = expression "in" expression ;
 */
class RangeCondition : public WhenCondition {
public:
  explicit RangeCondition(std::unique_ptr<Expression> value,
                          std::unique_ptr<Expression> range)
      : WhenCondition(NodeType::RANGE_CONDITION), value(std::move(value)),
        range(std::move(range)) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    return "RangeCondition(" + value->to_string() + " in " +
           range->to_string() + ")";
  }

  const Expression &get_value() const { return *value; }
  const Expression &get_range() const { return *range; }

  bool matches(const Expression &testValue [[maybe_unused]]) const override {
    // Implementation would check if testValue is in range
    return false; // Placeholder
  }

private:
  std::unique_ptr<Expression> value;
  std::unique_ptr<Expression> range;
};

/**
 * @brief Type condition for when expressions (is operator)
 *
 * EBNF: type_condition = expression "is" type_literal ;
 */
class TypeCondition : public WhenCondition {
public:
  explicit TypeCondition(std::unique_ptr<Expression> value,
                         std::unique_ptr<TypeLiteral> type)
      : WhenCondition(NodeType::TYPE_CONDITION), value(std::move(value)),
        type(std::move(type)) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    return "TypeCondition(" + value->to_string() + " is " + type->to_string() +
           ")";
  }

  const Expression &get_value() const { return *value; }
  const TypeLiteral &get_type() const { return *type; }

  bool matches(const Expression &testValue [[maybe_unused]]) const override {
    // Implementation would check type compatibility
    return false; // Placeholder
  }

private:
  std::unique_ptr<Expression> value;
  std::unique_ptr<TypeLiteral> type;
};

/**
 * @brief Guard condition for when expressions (condition with if clause)
 *
 * EBNF: guard_condition = value_condition "if" expression ;
 */
class GuardCondition : public WhenCondition {
public:
  explicit GuardCondition(std::unique_ptr<WhenCondition> innerCondition,
                          std::unique_ptr<Expression> guard)
      : WhenCondition(NodeType::GUARD_CONDITION),
        innerCondition(std::move(innerCondition)), guard(std::move(guard)) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    return "GuardCondition(" + innerCondition->to_string() + " if " +
           guard->to_string() + ")";
  }

  const WhenCondition &get_inner_condition() const { return *innerCondition; }
  const Expression &get_guard() const { return *guard; }

  bool matches(const Expression &testValue) const override {
    // Implementation would check both inner condition and guard
    return innerCondition->matches(testValue); // Simplified
  }

private:
  std::unique_ptr<WhenCondition> innerCondition;
  std::unique_ptr<Expression> guard;
};

/**
 * @brief Multiple condition for when expressions (comma-separated conditions)
 *
 * EBNF: multiple_condition = value_condition { "," value_condition } ;
 */
class MultipleCondition : public WhenCondition {
public:
  explicit MultipleCondition() : WhenCondition(NodeType::MULTIPLE_CONDITION) {}

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override {
    return "MultipleCondition(" + std::to_string(conditions.size()) +
           " conditions)";
  }

  void add_condition(std::unique_ptr<WhenCondition> condition) {
    conditions.push_back(std::move(condition));
  }

  const std::vector<std::unique_ptr<WhenCondition>> &get_conditions() const {
    return conditions;
  }

  bool matches(const Expression &testValue) const override {
    for (const auto &condition : conditions) {
      if (condition->matches(testValue)) {
        return true;
      }
    }
    return false;
  }

private:
  std::vector<std::unique_ptr<WhenCondition>> conditions;
};

} // namespace ast
} // namespace nugdev