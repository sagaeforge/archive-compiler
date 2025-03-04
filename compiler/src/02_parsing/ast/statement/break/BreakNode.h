#pragma once

#include "01_tokenize/Token.h"
#include "02_parsing/ast/AST.h"

namespace nugdev::compiler::ast::statement {

class BreakNode : public Statement {
  public:
    using self_t = BreakNode;

  public:
    BreakNode(const tokenize::Token &token, std::shared_ptr<Expression> label);

  public:
    virtual json::JsonValue to_json(json::JsonAllocator &allocator) const override;
    virtual icu::UnicodeString to_str() const override;
    virtual const tokenize::Token &get_token() const override;

  public:
    self_t set_label(std::shared_ptr<Expression> label);
    const std::shared_ptr<Expression> &get_label() const;

  private:
    tokenize::Token m_token;
    std::shared_ptr<Expression> m_label;
};

} // namespace nugdev::compiler::ast::statement
