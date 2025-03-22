#pragma once

#include "02_parsing/ParseStrategy.h"
#include "02_parsing/ast/AST.h"
#include <optional>
#include <unicode/unistr.h>

namespace nugdev::compiler::ast::expression {

class InfixExpressionNodeParseStrategy : public parsing::InfixParseStrategy {
  public:
    virtual bool can_parse(const parsing::Parser &parser, const tokenize::TokenStream &tokens) override;
    virtual parsing::ParseStrategyResult parse(parsing::Parser &parser, const tokenize::TokenStream &tokens) override;
    virtual parsing::ParseStrategyResult parse(parsing::Parser &parser, const tokenize::TokenStream &tokens, ExpressionPtr left) override;

    ASTNodePtr create_node(const tokenize::Token &token, ExpressionPtr left, ExpressionPtr right, std::optional<lib::String> op = std::nullopt);
};

} // namespace nugdev::compiler::ast::expression