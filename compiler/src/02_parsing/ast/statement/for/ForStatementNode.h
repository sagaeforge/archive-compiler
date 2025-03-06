#pragma once

#include "02_parsing/ast/AST.h"

namespace nugdev::compiler::ast::statement {

class ForStatementNode : public Statement {
  public:
    ForStatementNode(const tokenize::Token &token, std::shared_ptr<Expression> label, std::shared_ptr<Expression> init, std::shared_ptr<Expression> condition,
                     std::shared_ptr<Expression> post, std::shared_ptr<Statement> consequence);

  public:
    virtual icu::UnicodeString to_str() const override;
    virtual json::JsonValue to_json(json::JsonAllocator &allocator) const override;
    virtual const tokenize::Token &get_token() const override;

  public:
    const std::shared_ptr<Expression> &get_label() const;
    const std::shared_ptr<Expression> &get_init() const;
    const std::shared_ptr<Expression> &get_condition() const;
    const std::shared_ptr<Expression> &get_post() const;
    const std::shared_ptr<Statement> &get_consequence() const;

  private:
    tokenize::Token m_token;
    std::shared_ptr<Expression> m_label;
    std::shared_ptr<Expression> m_init;
    std::shared_ptr<Expression> m_condition;
    std::shared_ptr<Expression> m_post;
    std::shared_ptr<Statement> m_consequence;
};

} // namespace nugdev::compiler::ast::statement