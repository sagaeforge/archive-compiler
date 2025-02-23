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
};

} // namespace nugdev::compiler::parsing