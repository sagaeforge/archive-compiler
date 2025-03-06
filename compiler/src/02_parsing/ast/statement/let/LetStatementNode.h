#pragma once

#include "01_tokenize/Token.h"
#include "02_parsing/ast/AST.h"

namespace nugdev::compiler::ast::statement {

class LetStatementNode : public Statement {
  public:
    using self_t = LetStatementNode;

  public:
    LetStatementNode(const tokenize::Token &token, std::shared_ptr<Expression> name, std::shared_ptr<Expression> type, std::shared_ptr<Expression> value);

  public:
    virtual json::JsonValue to_json(json::JsonAllocator &allocator) const override;
    virtual icu::UnicodeString to_str() const override;
    virtual const tokenize::Token &get_token() const override;

  public:
    self_t set_type(std::shared_ptr<Expression> type);
    self_t set_value(std::shared_ptr<Expression> value);
    const std::shared_ptr<Expression> &get_name() const;
    const std::shared_ptr<Expression> &get_type() const;
    const std::shared_ptr<Expression> &get_value() const;

  private:
    tokenize::Token m_token;
    std::shared_ptr<Expression> m_name;
    std::shared_ptr<Expression> m_type;
    std::shared_ptr<Expression> m_value;
};

} // namespace nugdev::compiler::ast::statement