#pragma once

#include <map>
#include <variant>

#include "02_parsing/ast/AST.h"

namespace nugdev::compiler::ast::expression {

/*
    when {
        condition -> consequence
        else -> alternative // 이건 옵션
    }

    when (target-literal) {
        condition -> consequence
        (in) expression -> consequence
        else -> alternative // 이건 옵션
    }

*/

class WhenNode : public Expression {
  public:
    using Consequence = std::variant<std::shared_ptr<Expression>, std::shared_ptr<Statement>>;
    WhenNode(const tokenize::Token &token, std::shared_ptr<Expression> target, std::map<std::shared_ptr<Expression>, Consequence> conditions,
             Consequence alternative);

  public:
    virtual icu::UnicodeString to_str() const override;
    virtual json::JsonValue to_json(json::JsonAllocator &allocator) const override;
    virtual const tokenize::Token &get_token() const override;

  private:
    tokenize::Token m_token;
    std::shared_ptr<Expression> m_target;                            // 조건 대상인데, 있으면 condition은 infix 형태로 전환됨.
    std::map<std::shared_ptr<Expression>, Consequence> m_conditions; // consequence는 statement 혹은 expression
    Consequence m_alternative;
};

} // namespace nugdev::compiler::ast::expression