#pragma once

#include "01_tokenize/Token.h"
#include "02_parsing/ast/AST.h"

namespace nugdev::compiler::ast::statement {

class ReturnStatementNode : public Statement {
  public:
    using self_t = ReturnStatementNode *;

  public:
    ReturnStatementNode(const tokenize::Token &token, std::shared_ptr<Expression> label, std::shared_ptr<Expression> value);

  public:
    json::JsonValue to_json(json::JsonAllocator &allocator) const override;
    icu::UnicodeString to_str() const override;
    const tokenize::Token &get_token() const override;

  public:
    self_t set_label(std::shared_ptr<Expression> label);
    self_t set_value(std::shared_ptr<Expression> value);
    const std::shared_ptr<Expression> &get_label() const;
    const std::shared_ptr<Expression> &get_value() const;

  private:
    tokenize::Token m_token;
    std::shared_ptr<Expression> m_label;
    std::shared_ptr<Expression> m_value;
};

} // namespace nugdev::compiler::ast::statement