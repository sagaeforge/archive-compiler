#pragma once

#include "02_parsing/ParseStrategy.h"
#include <optional>
#include <unicode/unistr.h>

namespace nugdev::compiler::ast::expression {

class InfixExpressionNodeParseStrategy : public parsing::InfixParseStrategy {
  public:
    virtual bool can_parse(const tokenize::TokenStream &tokens) override;
    virtual parsing::ParseStrategyResult parse(const tokenize::TokenStream &tokens) override;
    virtual parsing::ParseStrategyResult parse(const tokenize::TokenStream &tokens, std::shared_ptr<Expression> left) override;

    std::shared_ptr<ast::ASTNode> create_node(const tokenize::Token &token, std::shared_ptr<Expression> left, std::shared_ptr<Expression> right,
                                              std::optional<icu::UnicodeString> op = std::nullopt);
};

} // namespace nugdev::compiler::ast::expression