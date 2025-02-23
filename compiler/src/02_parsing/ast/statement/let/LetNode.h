#pragma once

#include "01_tokenize/Token.h"
#include "02_parsing/ast/AST.h"

namespace nugdev::compiler::ast::statement {

class LetNode : public Statement {
  public:
    LetNode(const tokenize::Token &token, std::shared_ptr<Expression> name, std::shared_ptr<Expression> type, std::shared_ptr<Expression> value);

  public:
    virtual json::JsonValue to_json(json::JsonAllocator &allocator) const override;
    virtual icu::UnicodeString to_str() const override;
    virtual const tokenize::Token &get_token() const override;

  private:
    tokenize::Token m_token;
    std::shared_ptr<Expression> m_name;
    std::shared_ptr<Expression> m_type;
    std::shared_ptr<Expression> m_value;
};

} // namespace nugdev::compiler::ast::statement