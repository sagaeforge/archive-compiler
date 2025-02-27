#pragma once

#include "01_tokenize/Token.h"
#include "02_parsing/ast/AST.h"

namespace nugdev::compiler::ast::statement {

class ReturnNode : public Statement {
  public:
    using self_t = ReturnNode *;

  public:
    ReturnNode(const tokenize::Token &token, std::shared_ptr<Expression> label, std::shared_ptr<Expression> value);

  public:
    json::JsonValue to_json(json::JsonAllocator &allocator) const override;
    icu::UnicodeString to_str() const override;
    const tokenize::Token &get_token() const override;

  public:
    self_t set_label(std::shared_ptr<Expression> label);
    self_t set_value(std::shared_ptr<Expression> value);

  private:
    tokenize::Token m_token;
    std::shared_ptr<Expression> m_label;
    std::shared_ptr<Expression> m_value;
};

} // namespace nugdev::compiler::ast::statement