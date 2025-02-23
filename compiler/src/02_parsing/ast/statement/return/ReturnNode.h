#pragma once

#include "01_tokenize/Token.h"
#include "02_parsing/ast/AST.h"

namespace nugdev::compiler::ast::statement {

class ReturnNode : public Statement {
  public:
    ReturnNode(tokenize::Token &token, std::shared_ptr<Expression> label, std::shared_ptr<Expression> return_expression);

  public:
    json::JsonValue to_json(json::JsonAllocator &allocator) const override;
    icu::UnicodeString to_str() const override;
    const tokenize::Token &get_token() const override;

  private:
    tokenize::Token m_token;
    std::shared_ptr<Expression> m_label;
    std::shared_ptr<Expression> m_returnExpression;
};

} // namespace nugdev::compiler::ast::statement