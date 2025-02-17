#include "02_parsing/Parser.h"
#include <memory>

namespace nugdev::compiler::parsing {

Precedence getPrecedence(tokenize::TokenType type) {
    switch (type) {
    case tokenize::TokenType::Eq:
    case tokenize::TokenType::NotEq:
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

Parser::Parser(const TokenStream &tokens) : stream(tokens) {
    prefixParseFns = {
        {tokenize::TokenType::Ident, [&]() { return this->parseIdentifier(); }},
        {tokenize::TokenType::Number, [&]() { return this->parseNumberLiteral(); }},
        {tokenize::TokenType::String, [&]() { return this->parseStringLiteral(); }},
        {tokenize::TokenType::Bang, [&]() { return this->parsePrefixExpression(); }},
        {tokenize::TokenType::Minus, [&]() { return this->parsePrefixExpression(); }},
        {tokenize::TokenType::True, [&]() { return this->parseBoolean(); }},
        {tokenize::TokenType::False, [&]() { return this->parseBoolean(); }},
        {tokenize::TokenType::LParen, [&]() { return this->parseGroupedExpression(); }},
        {tokenize::TokenType::If, [&]() { return this->parseIfExpression(); }},
        {tokenize::TokenType::Function, [&]() { return this->parseFunctionLiteral(); }},
        {tokenize::TokenType::LBracket, [&]() { return this->parseArrayLiteral(); }},
        {tokenize::TokenType::LBrace, [&]() { return this->parseHashLiteral(); }},
    };

    infixParseFns = {
        {tokenize::TokenType::Plus, [&](std::shared_ptr<ast::Expression> expression) { return this->parseInfixExpression(std::move(expression)); }},
        {tokenize::TokenType::Minus, [&](std::shared_ptr<ast::Expression> expression) { return this->parseInfixExpression(std::move(expression)); }},
        {tokenize::TokenType::Slash, [&](std::shared_ptr<ast::Expression> expression) { return this->parseInfixExpression(std::move(expression)); }},
        {tokenize::TokenType::Asterisk, [&](std::shared_ptr<ast::Expression> expression) { return this->parseInfixExpression(std::move(expression)); }},
        {tokenize::TokenType::Eq, [&](std::shared_ptr<ast::Expression> expression) { return this->parseInfixExpression(std::move(expression)); }},
        {tokenize::TokenType::NotEq, [&](std::shared_ptr<ast::Expression> expression) { return this->parseInfixExpression(std::move(expression)); }},
        {tokenize::TokenType::LessThan, [&](std::shared_ptr<ast::Expression> expression) { return this->parseInfixExpression(std::move(expression)); }},
        {tokenize::TokenType::GreaterThan, [&](std::shared_ptr<ast::Expression> expression) { return this->parseInfixExpression(std::move(expression)); }},

        {tokenize::TokenType::LParen, [&](std::shared_ptr<ast::Expression> expression) { return this->parseCallExpression(std::move(expression)); }},
        {tokenize::TokenType::LBracket, [&](std::shared_ptr<ast::Expression> expression) { return this->parseIndexExpression(std::move(expression)); }},
    };

    nextToken();
    nextToken();
}

void Parser::nextToken() {
    auto checkpoint = this->stream.checkpoint();
    this->stream.commit(checkpoint.next());
}

} // namespace nugdev::compiler::parsing
