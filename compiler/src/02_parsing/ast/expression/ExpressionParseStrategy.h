#pragma once

#include "01_tokenize/Token.h"
#include "02_parsing/ParseStrategy.h"

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
    using prefixParseFn = std::function<parsing::ParseStrategyResult(const tokenize::TokenStream &)>;
    using infixParseFn = std::function<parsing::ParseStrategyResult(const tokenize::TokenStream &, std::shared_ptr<ast::Expression>)>;

  public:
    ExpressionParseStrategy();
    ExpressionParseStrategy(std::unordered_map<tokenize::TokenType, prefixParseFn> prefixParseFns,
                            std::unordered_map<tokenize::TokenType, infixParseFn> infixParseFns);

  public:
    virtual bool can_parse(const tokenize::TokenStream &tokens) override;
    virtual parsing::ParseStrategyResult parse(const tokenize::TokenStream &tokens) override;
    parsing::ParseStrategyResult parse(const tokenize::TokenStream &tokens, Precedence precedence);

  private:
    std::unordered_map<tokenize::TokenType, prefixParseFn> m_prefixParseFns;
    std::unordered_map<tokenize::TokenType, infixParseFn> m_infixParseFns;
};

} // namespace nugdev::compiler::ast::expression