#pragma once

#include "01_tokenize/Token.h"
#include "02_parsing/ParseStrategy.h"
#include "02_parsing/Parser.h"

#include <unordered_map>

namespace nugdev::compiler::ast::expression {

class ExpressionParseStrategy : public parsing::ParseStrategy {

  public:
    ExpressionParseStrategy();
    ExpressionParseStrategy(std::unordered_map<tokenize::TokenType, std::shared_ptr<parsing::ParseStrategy>> prefixParseFns,
                            std::unordered_map<tokenize::TokenType, std::shared_ptr<parsing::ParseStrategy>> infixParseFns);

  public:
    virtual bool can_parse(const parsing::Parser &parser, const tokenize::TokenStream &tokens) override;
    virtual parsing::ParseStrategyResult parse(parsing::Parser &parser, const tokenize::TokenStream &tokens) override;
    parsing::ParseStrategyResult parse(parsing::Parser &parser, const tokenize::TokenStream &tokens, parsing::Precedence precedence);

  private:
    std::unordered_map<tokenize::TokenType, std::shared_ptr<parsing::ParseStrategy>> m_prefixParseFns;
    std::unordered_map<tokenize::TokenType, std::shared_ptr<parsing::ParseStrategy>> m_infixParseFns;
};

} // namespace nugdev::compiler::ast::expression