#pragma once

#include "01_tokenize/Token.h"
#include "02_parsing/ParseStrategy.h"

#include <unordered_map>

namespace nugdev::compiler::ast::expression {

class ExpressionParseStrategy : public parsing::ParseStrategy {
  public:
    enum class Precedence {
        Lowest = 1,
        Equals,      // ==
        LessGreater, // > or <
        Sum,         // +
        Product,     // *
        Prefix,      // -X or !X
        Call,        // myFunction(X)
        Index,       // array[index]
        Unknown,
    };
    static Precedence get_precedence(tokenize::TokenType type);

  public:
    ExpressionParseStrategy();
    ExpressionParseStrategy(std::unordered_map<tokenize::TokenType, std::shared_ptr<parsing::ParseStrategy>> prefixParseFns,
                            std::unordered_map<tokenize::TokenType, std::shared_ptr<parsing::ParseStrategy>> infixParseFns);

  public:
    virtual bool can_parse(const tokenize::TokenStream &tokens) override;
    virtual parsing::ParseStrategyResult parse(const tokenize::TokenStream &tokens) override;
    parsing::ParseStrategyResult parse(const tokenize::TokenStream &tokens, Precedence precedence);

  private:
    std::unordered_map<tokenize::TokenType, std::shared_ptr<parsing::ParseStrategy>> m_prefixParseFns;
    std::unordered_map<tokenize::TokenType, std::shared_ptr<parsing::ParseStrategy>> m_infixParseFns;
};

} // namespace nugdev::compiler::ast::expression