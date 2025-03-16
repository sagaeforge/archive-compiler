#pragma once

#include "01_tokenize/Token.h"
#include "02_parsing/ast/AST.h"

namespace nugdev::compiler::ast {

namespace expression {
class TypeLiteralNode;
} // namespace expression

namespace statement {

class LetStatementNode : public Statement {
  public:
    using self_t = LetStatementNode;

  public:
    LetStatementNode(const tokenize::Token &token, const std::shared_ptr<Expression> &name, const std::shared_ptr<expression::TypeLiteralNode> &type,
                     const std::shared_ptr<Expression> &value);

  public:
    virtual const tokenize::Token &get_token() const override;

  public:
    const std::shared_ptr<Expression> &get_name() const;
    const std::shared_ptr<expression::TypeLiteralNode> &get_type() const;
    const std::shared_ptr<Expression> &get_value() const;

  private:
    tokenize::Token m_token;
    std::shared_ptr<Expression> m_name;
    std::shared_ptr<expression::TypeLiteralNode> m_type;
    std::shared_ptr<Expression> m_value;
};

} // namespace statement
} // namespace nugdev::compiler::ast