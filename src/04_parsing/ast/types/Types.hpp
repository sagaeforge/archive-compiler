#pragma once

#include <04_parsing/ast/core/ASTNode.hpp>
#include <04_parsing/ast/core/ASTVisitor.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace nugdev {
namespace ast {

// Forward declaration for visitor
class ASTVisitor;

/**
 * @brief Base class for type literals
 *
 * EBNF: type_literal = simple_type | function_type | optional_type | tuple_type
 */
class TypeLiteral : public ASTNode {
public:
  explicit TypeLiteral();
  virtual ~TypeLiteral() = default;

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  std::string to_string() const override;

  // Pure virtual methods
  virtual bool is_compatible_with(const TypeLiteral &other) const = 0;
  virtual std::string get_type_name() const = 0;
};

/**
 * @brief Simple type: identifier
 *
 * EBNF: simple_type = IDENTIFIER ;
 */
class SimpleType : public TypeLiteral {
public:
  explicit SimpleType(const std::string &name);

  std::string get_type_name() const override;
  bool is_compatible_with(const TypeLiteral &other) const override;
  std::string to_string() const override;

private:
  std::string typeName;
};

/**
 * @brief Function type: (param_types...) -> return_type
 *
 * EBNF: function_type = "(" [ type_list ] ")" "->" type_literal ;
 */
class FunctionType : public TypeLiteral {
public:
  explicit FunctionType(
      std::vector<std::unique_ptr<TypeLiteral>> parameterTypes,
      std::unique_ptr<TypeLiteral> returnType);

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  const std::vector<std::unique_ptr<TypeLiteral>> &get_parameter_types() const;
  const TypeLiteral &get_return_type() const;

  std::string get_type_name() const override;
  bool is_compatible_with(const TypeLiteral &other) const override;
  std::string to_string() const override;

private:
  std::vector<std::unique_ptr<TypeLiteral>> parameterTypes;
  std::unique_ptr<TypeLiteral> returnType;
};

/**
 * @brief Optional type: type_literal "?"
 *
 * EBNF: optional_type = type_literal "?" ;
 */
class OptionalType : public TypeLiteral {
public:
  explicit OptionalType(std::unique_ptr<TypeLiteral> innerType);

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  const TypeLiteral &get_inner_type() const;

  std::string get_type_name() const override;
  bool is_compatible_with(const TypeLiteral &other) const override;
  std::string to_string() const override;

private:
  std::unique_ptr<TypeLiteral> innerType;
};

/**
 * @brief Tuple type: (type1, type2, ...)
 *
 * EBNF: tuple_type = "(" type_literal "," type_list ")" ;
 * Note: Tuple requires minimum 2 elements with comma
 */
class TupleType : public TypeLiteral {
public:
  explicit TupleType(std::vector<std::unique_ptr<TypeLiteral>> elementTypes);

  void accept(ASTVisitor &visitor) override;
  void accept(ASTVisitor &visitor) const override;

  const std::vector<std::unique_ptr<TypeLiteral>> &get_element_types() const;
  size_t get_element_count() const;
  const TypeLiteral &get_element_type(size_t index) const;

  std::string get_type_name() const override;
  bool is_compatible_with(const TypeLiteral &other) const override;
  std::string to_string() const override;

private:
  std::vector<std::unique_ptr<TypeLiteral>> elementTypes;
};

// Factory functions (template-like, keep inline)
namespace TypeFactory {
inline std::unique_ptr<SimpleType> create_simple_type(const std::string &name) {
  return std::make_unique<SimpleType>(name);
}

inline std::unique_ptr<OptionalType>
create_optional_type(std::unique_ptr<TypeLiteral> innerType) {
  return std::make_unique<OptionalType>(std::move(innerType));
}

inline std::unique_ptr<FunctionType>
create_function_type(std::vector<std::unique_ptr<TypeLiteral>> paramTypes,
                     std::unique_ptr<TypeLiteral> returnType) {
  return std::make_unique<FunctionType>(std::move(paramTypes),
                                        std::move(returnType));
}

inline std::unique_ptr<TupleType>
create_tuple_type(std::vector<std::unique_ptr<TypeLiteral>> elementTypes) {
  return std::make_unique<TupleType>(std::move(elementTypes));
}
} // namespace TypeFactory

} // namespace ast
} // namespace nugdev