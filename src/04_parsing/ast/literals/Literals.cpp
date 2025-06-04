#include "Literals.hpp"
#include <04_parsing/ast/core/ASTVisitor.hpp>
#include <sstream>
#include <stdexcept>

namespace nugdev {
namespace ast {

// NumberLiteral implementations
void NumberLiteral::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void NumberLiteral::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<NumberLiteral &>(*this));
}

int64_t NumberLiteral::get_integer_value() const {
  if (is_floating_point()) {
    throw std::runtime_error(
        "Cannot convert floating point literal to integer");
  }

  std::string cleanValue = value;
  // Remove underscores
  cleanValue.erase(std::remove(cleanValue.begin(), cleanValue.end(), '_'),
                   cleanValue.end());

  switch (numberType) {
  case NumberType::DECIMAL_INTEGER:
    return std::stoll(cleanValue);
  case NumberType::BINARY_INTEGER:
    return std::stoll(cleanValue.substr(2), nullptr, 2); // Skip "0b"
  case NumberType::OCTAL_INTEGER:
    return std::stoll(cleanValue.substr(2), nullptr, 8); // Skip "0o"
  case NumberType::HEXADECIMAL_INTEGER:
    return std::stoll(cleanValue.substr(2), nullptr, 16); // Skip "0x"
  default:
    throw std::runtime_error("Invalid number type for integer conversion");
  }
}

double NumberLiteral::get_floating_point_value() const {
  std::string cleanValue = value;
  // Remove underscores
  cleanValue.erase(std::remove(cleanValue.begin(), cleanValue.end(), '_'),
                   cleanValue.end());

  if (is_floating_point()) {
    return std::stod(cleanValue);
  } else {
    // Convert integer to double
    return static_cast<double>(get_integer_value());
  }
}

// StringLiteral implementations
void StringLiteral::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void StringLiteral::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<StringLiteral &>(*this));
}

// CharacterLiteral implementations
void CharacterLiteral::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void CharacterLiteral::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<CharacterLiteral &>(*this));
}

// BooleanLiteral implementations
void BooleanLiteral::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void BooleanLiteral::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<BooleanLiteral &>(*this));
}

// NullLiteral implementations
void NullLiteral::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void NullLiteral::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<NullLiteral &>(*this));
}

// NoneLiteral implementations
void NoneLiteral::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void NoneLiteral::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<NoneLiteral &>(*this));
}

// RangeLiteral implementations
void RangeLiteral::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void RangeLiteral::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<RangeLiteral &>(*this));
}

// ArrayLiteral implementations
void ArrayLiteral::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void ArrayLiteral::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<ArrayLiteral &>(*this));
}

// ObjectLiteral implementations
void ObjectLiteral::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void ObjectLiteral::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<ObjectLiteral &>(*this));
}

// ObjectProperty implementations
void ObjectProperty::accept(ASTVisitor &visitor) { visitor.visit(*this); }

void ObjectProperty::accept(ASTVisitor &visitor) const {
  visitor.visit(const_cast<ObjectProperty &>(*this));
}

} // namespace ast
} // namespace nugdev