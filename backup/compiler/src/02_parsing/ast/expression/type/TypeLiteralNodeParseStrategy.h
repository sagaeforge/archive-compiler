#pragma once

#include "02_parsing/ParseStrategy.h"
#include "02_parsing/ast/AST.h"

namespace nugdev::compiler::ast::expression {

class TypeLiteralNodeParseStrategy : public parsing::ParseStrategy {
  public:
    virtual bool can_parse(const parsing::Parser &parser, const tokenize::TokenStream &tokens) override;
    virtual parsing::ParseStrategyResult parse(parsing::Parser &parser, const tokenize::TokenStream &tokens) override;

    ASTNodePtr create_node(const tokenize::Token &token, const TypeInfo &meta);
};

} // namespace nugdev::compiler::ast::expression
