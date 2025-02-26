#pragma once

#include "01_tokenize/Token.h"
#include "02_parsing/ast/AST.h"

namespace nugdev::compiler::parsing {

struct ParseStrategyResult {
    std::shared_ptr<ast::ASTNode> node;
    tokenize::TokenStreamIterator next_position;
};

class ParseStrategy {
  public:
    virtual bool can_parse(const tokenize::TokenStream &tokens) = 0;
    virtual ParseStrategyResult parse(const tokenize::TokenStream &tokens) = 0;

    bool contains(const tokenize::TokenStreamIterator &itr, const std::vector<tokenize::TokenType> &types);
};

class InfixParseStrategy : public ParseStrategy {
  public:
    virtual ParseStrategyResult parse(const tokenize::TokenStream &tokens, std::shared_ptr<ast::Expression> left) = 0;
};

} // namespace nugdev::compiler::parsing