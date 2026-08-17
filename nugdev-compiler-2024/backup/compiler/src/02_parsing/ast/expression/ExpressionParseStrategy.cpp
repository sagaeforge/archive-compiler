#include "ExpressionParseStrategy.h"

#include "01_tokenize/Token.h"
#include "02_parsing/Parser.h"
#include "02_parsing/ast/expression/GroupExpressionParseStrategy.h"
#include "02_parsing/ast/expression/array/ArrayLiteralNodeParseStrategy.h"
#include "02_parsing/ast/expression/boolean/BooleanLiteralNodeParseStrategy.h"
#include "02_parsing/ast/expression/call/CallExpressionNodeParseStrategy.h"
#include "02_parsing/ast/expression/function/FunctionExpressionNodeParseStrategy.h"
#include "02_parsing/ast/expression/identifier/IdentifierLiteralNodeParseStrategy.h"
#include "02_parsing/ast/expression/if/IfExpressionNodeParseStrategy.h"
#include "02_parsing/ast/expression/index/IndexExpressionNodeParseStrategy.h"
#include "02_parsing/ast/expression/infix/InfixExpressionNodeParseStrategy.h"
#include "02_parsing/ast/expression/number/NumberLiteralNodeParseStrategy.h"
#include "02_parsing/ast/expression/post/PostExpressionNodeParseStrategy.h"
#include "02_parsing/ast/expression/prefix/PrefixExpressionNodeParseStrategy.h"
#include "02_parsing/ast/expression/string/StringLiteralNodeParseStrategy.h"
#include "02_parsing/ast/expression/when/WhenExpressionNodeParseStrategy.h"

namespace nugdev::compiler::ast::expression {

ExpressionParseStrategy::ExpressionParseStrategy() {
    m_prefixParseFns = {
        {tokenize::TokenType::Ident, std::make_shared<IdentifierLiteralNodeParseStrategy>()},
        {tokenize::TokenType::Number, std::make_shared<NumberLiteralNodeParseStrategy>()},
        {tokenize::TokenType::String, std::make_shared<StringLiteralNodeParseStrategy>()},
        {tokenize::TokenType::Plus, std::make_shared<PrefixExpressionNodeParseStrategy>()},
        {tokenize::TokenType::ExclamationMark, std::make_shared<PrefixExpressionNodeParseStrategy>()},
        {tokenize::TokenType::Minus, std::make_shared<PrefixExpressionNodeParseStrategy>()},
        {tokenize::TokenType::True, std::make_shared<BooleanLiteralNodeParseStrategy>()},
        {tokenize::TokenType::False, std::make_shared<BooleanLiteralNodeParseStrategy>()},
        {tokenize::TokenType::LParen, std::make_shared<GroupExpressionParseStrategy>()},
        {tokenize::TokenType::If, std::make_shared<IfExpressionNodeParseStrategy>()},
        {tokenize::TokenType::Function, std::make_shared<FunctionExpressionNodeParseStrategy>()},
        {tokenize::TokenType::LBracket, std::make_shared<ArrayLiteralNodeParseStrategy>()},
        {tokenize::TokenType::When, std::make_shared<WhenExpressionNodeParseStrategy>()},
        {tokenize::TokenType::Inc, std::make_shared<PrefixExpressionNodeParseStrategy>()},
        {tokenize::TokenType::Dec, std::make_shared<PrefixExpressionNodeParseStrategy>()},
    };
    m_infixParseFns = {
        {tokenize::TokenType::Plus, std::make_shared<InfixExpressionNodeParseStrategy>()},
        {tokenize::TokenType::Minus, std::make_shared<InfixExpressionNodeParseStrategy>()},
        {tokenize::TokenType::Slash, std::make_shared<InfixExpressionNodeParseStrategy>()},
        {tokenize::TokenType::Asterisk, std::make_shared<InfixExpressionNodeParseStrategy>()},
        {tokenize::TokenType::Equal, std::make_shared<InfixExpressionNodeParseStrategy>()},
        {tokenize::TokenType::NotEqual, std::make_shared<InfixExpressionNodeParseStrategy>()},
        {tokenize::TokenType::LessThan, std::make_shared<InfixExpressionNodeParseStrategy>()},
        {tokenize::TokenType::GreaterThan, std::make_shared<InfixExpressionNodeParseStrategy>()},
        {tokenize::TokenType::In, std::make_shared<InfixExpressionNodeParseStrategy>()},

        {tokenize::TokenType::LParen, std::make_shared<CallExpressionNodeParseStrategy>()},
        {tokenize::TokenType::LBracket, std::make_shared<IndexExpressionNodeParseStrategy>()},

        {tokenize::TokenType::Inc, std::make_shared<PostExpressionNodeParseStrategy>()},
        {tokenize::TokenType::Dec, std::make_shared<PostExpressionNodeParseStrategy>()},
    };
}

ExpressionParseStrategy::ExpressionParseStrategy(std::unordered_map<tokenize::TokenType, std::shared_ptr<parsing::ParseStrategy>> prefixParseFns,
                                                 std::unordered_map<tokenize::TokenType, std::shared_ptr<parsing::ParseStrategy>> infixParseFns) {
    m_prefixParseFns = prefixParseFns;
    m_infixParseFns = infixParseFns;
}

bool ExpressionParseStrategy::can_parse(const parsing::Parser &parser, const tokenize::TokenStream &tokens) {
    for (auto &[type, parseFn] : m_prefixParseFns) {
        if (type == tokens.current()->get_type()) {
            return true;
        }
    }
    return false;
}

parsing::ParseStrategyResult ExpressionParseStrategy::parse(parsing::Parser &parser, const tokenize::TokenStream &tokens) {
    return parse(parser, tokens, parsing::Precedence::Lowest);
}

parsing::ParseStrategyResult ExpressionParseStrategy::parse(parsing::Parser &parser, const tokenize::TokenStream &tokens, parsing::Precedence precedence) {
    auto workbench = tokens.clone();
    if (workbench.current().valid() == false) {
        throw std::runtime_error("Invalid token stream");
    }

    auto prefix = m_prefixParseFns.find(workbench.current()->get_type());
    if (prefix == m_prefixParseFns.end()) {
        throw std::runtime_error("Invalid token stream");
    }

    auto [leftExpr, leftMoveItr] = prefix->second->parse(parser, workbench);
    workbench.move_at(leftMoveItr);
    while (workbench.current().valid() && precedence < parser.get_precedence(workbench.current()->get_type())) {
        auto infix = m_infixParseFns.find(workbench.current()->get_type());
        if (infix == m_infixParseFns.end()) {
            return parsing::ParseStrategyResult{leftExpr, tokens.current()};
        }

        auto infixParseStrategy = std::dynamic_pointer_cast<parsing::InfixParseStrategy>(infix->second);
        auto [rightExpr, rightMoveItr] = infixParseStrategy->parse(parser, workbench, leftExpr->as<ast::Expression>());
        workbench.move_at(rightMoveItr);
        leftExpr = rightExpr;
    }

    return parsing::ParseStrategyResult{leftExpr, tokens.begin() + workbench.current().distance()};
}

} // namespace nugdev::compiler::ast::expression
