#pragma once

#include "02_parsing/ast/AST.h"

namespace nugdev::compiler::ast::expression {

class TypeLiteralNode : public Expression {

  public:
    TypeLiteralNode(const tokenize::Token &token, const TypeInfo &meta);

  public:
    virtual const tokenize::Token &get_token() const override;
    const TypeInfo &get_meta() const;

  private:
    tokenize::Token m_token;
    TypeInfo m_meta;
};

} // namespace nugdev::compiler::ast::expression