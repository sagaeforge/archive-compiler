#include "ExpressionParseStrategy.h"

#include "01_tokenize/Token.h"
#include "02_parsing/ast/expression/GroupExpressionParseStrategy.h"
#include "02_parsing/ast/expression/array/ArrayLiteralNodeParseStrategy.h"
#include "02_parsing/ast/expression/boolean/BooleanLiteralNodeParseStrategy.h"
#include "02_parsing/ast/expression/call/CallExpressionNodeParseStrategy.h"
#include "02_parsing/ast/expression/function/FunctionLiteralNodeParseStrategy.h"
#include "02_parsing/ast/expression/identifier/IdentifierLiteralNodeParseStrategy.h"
#include "02_parsing/ast/expression/if/IfExpressionNodeParseStrategy.h"
#include "02_parsing/ast/expression/index/IndexExpressionNodeParseStrategy.h"
#include "02_parsing/ast/expression/infix/InfixExpressionNodeParseStrategy.h"
#include "02_parsing/ast/expression/number/NumberLiteralNodeParseStrategy.h"
#include "02_parsing/ast/expression/prefix/PrefixExpressionNodeParseStrategy.h"
#include "02_parsing/ast/expression/string/StringLiteralNodeParseStrategy.h"

namespace nugdev::compiler::ast::expression {

ExpressionParseStrategy::ExpressionParseStrategy() {
    m_prefixParseFns = {
        {tokenize::TokenType::Ident, std::make_shared<IdentifierLiteralNodeParseStrategy>()},
        {tokenize::TokenType::Number, std::make_shared<NumberLiteralNodeParseStrategy>()},
        {tokenize::TokenType::String, std::make_shared<StringLiteralNodeParseStrategy>()},
        {tokenize::TokenType::ExclamationMark, std::make_shared<BooleanLiteralNodeParseStrategy>()},
        {tokenize::TokenType::Minus, std::make_shared<PrefixExpressionNodeParseStrategy>()},
        {tokenize::TokenType::True, std::make_shared<BooleanLiteralNodeParseStrategy>()},
        {tokenize::TokenType::False, std::make_shared<BooleanLiteralNodeParseStrategy>()},
        {tokenize::TokenType::LParen, std::make_shared<GroupExpressionParseStrategy>()},
        {tokenize::TokenType::If, std::make_shared<IfExpressionNodeParseStrategy>()},
        {tokenize::TokenType::Function, std::make_shared<FunctionLiteralNodeParseStrategy>()},
        {tokenize::TokenType::LBracket, std::make_shared<ArrayLiteralNodeParseStrategy>()},
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

        {tokenize::TokenType::LParen, std::make_shared<CallExpressionNodeParseStrategy>()},
        {tokenize::TokenType::LBracket, std::make_shared<IndexExpressionNodeParseStrategy>()},
    };
}

ExpressionParseStrategy::ExpressionParseStrategy(std::unordered_map<tokenize::TokenType, std::shared_ptr<parsing::ParseStrategy>> prefixParseFns,
                                                 std::unordered_map<tokenize::TokenType, std::shared_ptr<parsing::ParseStrategy>> infixParseFns) {
    m_prefixParseFns = prefixParseFns;
    m_infixParseFns = infixParseFns;
}

ExpressionParseStrategy::Precedence ExpressionParseStrategy::get_precedence(tokenize::TokenType type) {
    switch (type) {
    case tokenize::TokenType::Equal:
    case tokenize::TokenType::NotEqual:
        return Precedence::Equals;
    case tokenize::TokenType::LessThan:
    case tokenize::TokenType::GreaterThan:
        return Precedence::LessGreater;
    case tokenize::TokenType::Plus:
    case tokenize::TokenType::Minus:
        return Precedence::Sum;
    case tokenize::TokenType::Asterisk:
    case tokenize::TokenType::Slash:
        return Precedence::Product;
    case tokenize::TokenType::LParen:
        return Precedence::Call;
    case tokenize::TokenType::LBracket:
        return Precedence::Index;
    default:
        return Precedence::Unknown;
    }
}

bool ExpressionParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return true; }

parsing::ParseStrategyResult ExpressionParseStrategy::parse(const tokenize::TokenStream &tokens) { return parse(tokens, Precedence::Lowest); }

parsing::ParseStrategyResult ExpressionParseStrategy::parse(const tokenize::TokenStream &tokens, Precedence precedence) {
    auto workbench = tokens.clone();
    auto prefix = m_prefixParseFns.find(workbench.current()->get_type());
    if (prefix == m_prefixParseFns.end()) {
        throw std::runtime_error("Invalid token stream");
    }

    auto [leftExpr, leftMoveItr] = prefix->second->parse(workbench);
    workbench.move_at(leftMoveItr);
    while (workbench.current().valid() && precedence < get_precedence(workbench.current()->get_type())) {
        auto infix = m_infixParseFns.find(workbench.current()->get_type());
        if (infix == m_infixParseFns.end()) {
            return parsing::ParseStrategyResult{leftExpr, tokens.current()};
        }
        workbench.next();

        auto infixParseStrategy = std::dynamic_pointer_cast<InfixExpressionNodeParseStrategy>(infix->second);
        auto [rightExpr, rightMoveItr] = infixParseStrategy->parse(workbench, leftExpr->as<ast::Expression>());
        workbench.move_at(rightMoveItr);
        leftExpr = rightExpr;
    }

    return parsing::ParseStrategyResult{leftExpr, workbench.current() + workbench.current().distance()};
}

} // namespace nugdev::compiler::ast::expression
