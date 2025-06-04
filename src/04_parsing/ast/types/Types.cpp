#include <04_parsing/ast/core/ASTVisitor.hpp>
#include <04_parsing/ast/types/Types.hpp>

namespace nugdev {
namespace ast {

// TypeLiteral implementation
TypeLiteral::TypeLiteral() : ASTNode(NodeType::TYPE_LITERAL) {}

std::string TypeLiteral::to_string() const { return "TypeLiteral"; }

// TypeLiteral visitor implementations
void TypeLiteral::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void TypeLiteral::accept(ASTVisitor &visitor) const {
  // For const visitor, we need a different approach
  // The visitor interface needs to be designed to handle const visits
  visitor.visit(const_cast<TypeLiteral &>(*this));
}

// SimpleType implementation
SimpleType::SimpleType(const std::string &name)
    : TypeLiteral(), typeName(name) {}

std::string SimpleType::get_type_name() const { return typeName; }

bool SimpleType::is_compatible_with(const TypeLiteral &other) const {
  const auto *otherSimple = dynamic_cast<const SimpleType *>(&other);
  if (!otherSimple)
    return false;
  return typeName == otherSimple->typeName;
}

std::string SimpleType::to_string() const {
  return "SimpleType(" + typeName + ")";
}

// FunctionType implementation
FunctionType::FunctionType(
    std::vector<std::unique_ptr<TypeLiteral>> parameterTypes,
    std::unique_ptr<TypeLiteral> returnType)
    : TypeLiteral(), parameterTypes(std::move(parameterTypes)),
      returnType(std::move(returnType)) {
  nodeType = NodeType::FUNCTION_TYPE;
}

const std::vector<std::unique_ptr<TypeLiteral>> &
FunctionType::get_parameter_types() const {
  return parameterTypes;
}

const TypeLiteral &FunctionType::get_return_type() const { return *returnType; }

std::string FunctionType::get_type_name() const {
  std::string result = "(";
  for (size_t i = 0; i < parameterTypes.size(); ++i) {
    if (i > 0)
      result += ", ";
    result += parameterTypes[i]->get_type_name();
  }
  result += ") -> " + returnType->get_type_name();
  return result;
}

bool FunctionType::is_compatible_with(const TypeLiteral &other) const {
  const auto *otherFunc = dynamic_cast<const FunctionType *>(&other);
  if (!otherFunc)
    return false;

  // Check parameter count
  if (parameterTypes.size() != otherFunc->parameterTypes.size()) {
    return false;
  }

  // Check parameter types (contravariant)
  for (size_t i = 0; i < parameterTypes.size(); ++i) {
    if (!otherFunc->parameterTypes[i]->is_compatible_with(*parameterTypes[i])) {
      return false;
    }
  }

  // Check return type (covariant)
  return returnType->is_compatible_with(*otherFunc->returnType);
}

std::string FunctionType::to_string() const {
  return "FunctionType(" + get_type_name() + ")";
}

// FunctionType visitor implementations
void FunctionType::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void FunctionType::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<FunctionType &>(*this));
}

// OptionalType implementation
OptionalType::OptionalType(std::unique_ptr<TypeLiteral> innerType)
    : TypeLiteral(), innerType(std::move(innerType)) {
  nodeType = NodeType::OPTIONAL_TYPE;
}

const TypeLiteral &OptionalType::get_inner_type() const { return *innerType; }

std::string OptionalType::get_type_name() const {
  return innerType->get_type_name() + "?";
}

bool OptionalType::is_compatible_with(const TypeLiteral &other) const {
  // Optional type is compatible with its inner type (nullable assignment)
  if (innerType->is_compatible_with(other)) {
    return true;
  }

  // Optional type is compatible with another optional of compatible inner type
  if (const auto *otherOptional = dynamic_cast<const OptionalType *>(&other)) {
    return innerType->is_compatible_with(otherOptional->get_inner_type());
  }

  return false;
}

std::string OptionalType::to_string() const {
  return "OptionalType(" + innerType->to_string() + ")";
}

// OptionalType visitor implementations
void OptionalType::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void OptionalType::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<OptionalType &>(*this));
}

// TupleType implementation
TupleType::TupleType(std::vector<std::unique_ptr<TypeLiteral>> elementTypes)
    : TypeLiteral(), elementTypes(std::move(elementTypes)) {
  nodeType = NodeType::TUPLE_TYPE;

  // Tuples must have at least 2 elements according to EBNF
  if (this->elementTypes.size() < 2) {
    throw std::invalid_argument("Tuple type must have at least 2 elements");
  }
}

const std::vector<std::unique_ptr<TypeLiteral>> &
TupleType::get_element_types() const {
  return elementTypes;
}

size_t TupleType::get_element_count() const { return elementTypes.size(); }

const TypeLiteral &TupleType::get_element_type(size_t index) const {
  if (index >= elementTypes.size()) {
    throw std::out_of_range("Tuple element index out of range");
  }
  return *elementTypes[index];
}

std::string TupleType::get_type_name() const {
  std::string result = "(";
  for (size_t i = 0; i < elementTypes.size(); ++i) {
    if (i > 0)
      result += ", ";
    result += elementTypes[i]->get_type_name();
  }
  result += ")";
  return result;
}

bool TupleType::is_compatible_with(const TypeLiteral &other) const {
  const auto *otherTuple = dynamic_cast<const TupleType *>(&other);
  if (!otherTuple)
    return false;

  if (elementTypes.size() != otherTuple->elementTypes.size()) {
    return false;
  }

  for (size_t i = 0; i < elementTypes.size(); ++i) {
    if (!elementTypes[i]->is_compatible_with(*otherTuple->elementTypes[i])) {
      return false;
    }
  }

  return true;
}

std::string TupleType::to_string() const {
  return "TupleType(" + get_type_name() + ")";
}

// TupleType visitor implementations
void TupleType::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void TupleType::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<TupleType &>(*this));
}

} // namespace ast
} // namespace nugdev