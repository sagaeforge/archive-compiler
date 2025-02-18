#include "02_parsing/Parser.h"

#include <memory>
#include <unicode/decimfmt.h>
#include <unicode/numfmt.h>

#include "01_tokenize/Token.h"

#include "02_parsing/ast/Expression/ArrayLiteral.h"
#include "02_parsing/ast/Expression/BooleanLiteral.h"
#include "02_parsing/ast/Expression/CallExpression.h"
#include "02_parsing/ast/Expression/FunctionLiteral.h"
#include "02_parsing/ast/Expression/IdentifierLiteral.h"
#include "02_parsing/ast/Expression/IfExpression.h"
#include "02_parsing/ast/Expression/IndexExpression.h"
#include "02_parsing/ast/Expression/InfixExpression.h"
#include "02_parsing/ast/Expression/NumberLiteral.h"
#include "02_parsing/ast/Expression/PrefixExpression.h"
#include "02_parsing/ast/Expression/StringLiteral.h"
#include "02_parsing/ast/Module/Program.h"
#include "02_parsing/ast/Statement/BlockStatement.h"
#include "02_parsing/ast/Statement/BreakStatement.h"
#include "02_parsing/ast/Statement/ContinueStatement.h"
#include "02_parsing/ast/Statement/ExpressionStatement.h"
#include "02_parsing/ast/Statement/LetStatement.h"
#include "02_parsing/ast/Statement/ReturnStatement.h"

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

Parser::Parser(const TokenStream &tokens) : stream(tokens), errors() {
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

tokenize::Token Parser::get_cur_token() const { return this->stream.checkpoint().value_or(tokenize::Token::Empty()); }

tokenize::Token Parser::get_peek_token() const { return (++this->stream.checkpoint()).value_or(tokenize::Token::Empty()); }

void Parser::nextToken() {
    auto checkpoint = this->stream.checkpoint();
    this->stream.commit(checkpoint.next());
}

bool Parser::curTokenIs(tokenize::TokenType type) { return this->get_cur_token().get_type() == type; }

bool Parser::peekTokenIs(tokenize::TokenType type) { return this->get_peek_token().get_type() == type; }

bool Parser::expectPeek(tokenize::TokenType type) {
    if (this->peekTokenIs(type)) {
        this->nextToken();
        return true;
    }

    this->errors.push_back(std::make_exception_ptr(ParserError()));
    return false;
}

std::vector<std::exception_ptr> Parser::getErrors() const { return this->errors; }

void Parser::noPrefixParseFnError(tokenize::TokenType type) { this->errors.push_back(std::make_exception_ptr(ParserError())); }

Precedence Parser::peekPrecedence() {
    auto checkpoint = this->stream.checkpoint();
    return getPrecedence(checkpoint.value_or(tokenize::Token::Empty()).get_type());
}

Precedence Parser::curPrecedence() {
    auto checkpoint = this->stream.checkpoint();
    return getPrecedence(checkpoint.value_or(tokenize::Token::Empty()).get_type());
}

std::shared_ptr<ast::Module> Parser::parseProgram() {
    auto statements = std::vector<std::shared_ptr<ast::Statement>>();

    while (this->curTokenIs(tokenize::TokenType::EoF) == false) {
        auto statement = this->parseStatement();
        if (statement != nullptr) {
            statements.push_back(statement);
        }
        this->nextToken();
    }

    return std::make_shared<ast::module::Program>(statements);
}

std::shared_ptr<ast::Statement> Parser::parseStatement() {
    switch (this->get_cur_token().get_type()) {
    case tokenize::TokenType::Let:
        return this->parseLetStatement();
    case tokenize::TokenType::Return:
        return this->parseReturnStatement();
    case tokenize::TokenType::Break:
        return this->parseBreakStatement();
    case tokenize::TokenType::Continue:
        return this->parseContinueStatement();
    default:
        return this->parseExpressionStatement();
    }
}

std::shared_ptr<ast::Statement> Parser::parseLetStatement() {
    auto token = this->get_cur_token();
    if (!this->expectPeek(tokenize::TokenType::Ident)) {
        return nullptr;
    }

    auto name = std::make_shared<ast::expression::IdentifierLiteral>(this->get_cur_token().get_literal());
    if (!this->expectPeek(tokenize::TokenType::Assign)) {
        return nullptr;
    }

    this->nextToken();
    auto value = this->parseExpression(Precedence::Lowest);
    if (this->peekTokenIs(tokenize::TokenType::SemiColon)) {
        this->nextToken();
    }

    return std::make_shared<ast::statement::LetStatement>(name, token, value);
}

std::shared_ptr<ast::Statement> Parser::parseReturnStatement() {
    auto token = this->get_cur_token();
    this->nextToken();

    auto returnValue = this->parseExpression(Precedence::Lowest);
    if (this->peekTokenIs(tokenize::TokenType::SemiColon)) {
        this->nextToken();
    }

    return std::make_shared<ast::statement::ReturnStatement>(token, returnValue);
}

std::shared_ptr<ast::Statement> Parser::parseBreakStatement() {
    auto token = this->get_cur_token();
    this->nextToken();
    if (this->peekTokenIs(tokenize::TokenType::SemiColon)) {
        this->nextToken();
    }

    return std::make_shared<ast::statement::BreakStatement>(token);
}

std::shared_ptr<ast::Statement> Parser::parseContinueStatement() {
    auto token = this->get_cur_token();
    this->nextToken();
    if (this->peekTokenIs(tokenize::TokenType::SemiColon)) {
        this->nextToken();
    }

    return std::make_shared<ast::statement::ContinueStatement>(token);
}

std::shared_ptr<ast::Statement> Parser::parseExpressionStatement() {
    auto token = this->get_cur_token();
    auto expression = this->parseExpression(Precedence::Lowest);

    if (this->peekTokenIs(tokenize::TokenType::SemiColon)) {
        this->nextToken();
    }

    return std::make_shared<ast::statement::ExpressionStatement>(token, expression);
}

std::shared_ptr<ast::Expression> Parser::parseExpression(Precedence precedence) {
    auto prefix = this->prefixParseFns.find(this->get_cur_token().get_type());
    if (prefix == this->prefixParseFns.end()) {
        this->noPrefixParseFnError(this->get_cur_token().get_type());
        return nullptr;
    }
    auto leftExp = prefix->second();

    while (!this->peekTokenIs(tokenize::TokenType::SemiColon) && precedence < this->peekPrecedence()) {
        auto infix = this->infixParseFns[this->get_peek_token().get_type()];
        if (infix == nullptr) {
            return leftExp;
        }

        this->nextToken();

        leftExp = infix(std::move(leftExp));
    }

    return leftExp;
}

std::shared_ptr<ast::Expression> Parser::parseIdentifier() { return std::make_shared<ast::expression::IdentifierLiteral>(this->get_cur_token().get_literal()); }

std::shared_ptr<ast::Expression> Parser::parseNumberLiteral() {
    auto token = this->get_cur_token();
    UErrorCode status = U_ZERO_ERROR;

    icu::UnicodeString pattern("#,##0.0#");

    std::unique_ptr<icu::DecimalFormat> fmt(new icu::DecimalFormat(pattern, status));

    if (U_FAILURE(status)) {
        return nullptr;
    }

    // 토큰의 리터럴을 UnicodeString으로 변환
    icu::UnicodeString numStr(token.get_literal());

    icu::Formattable result;

    // 파싱 수행
    fmt->parse(numStr, result, status);
    if (U_FAILURE(status)) {
        return nullptr;
    }

    // double 값 추출
    double value = result.getDouble(status);
    return std::make_shared<ast::expression::NumberLiteral>(token, value);
}

std::shared_ptr<ast::Expression> Parser::parsePrefixExpression() {
    auto token = this->get_cur_token();
    auto op = token.get_literal();

    this->nextToken();

    auto right = this->parseExpression(Precedence::Prefix);
    return std::make_shared<ast::expression::PrefixExpression>(token, op, right);
}

std::shared_ptr<ast::Expression> Parser::parseInfixExpression(std::shared_ptr<ast::Expression> left) {
    auto token = this->get_cur_token();
    auto op = token.get_literal();
    auto precedence = this->curPrecedence();

    this->nextToken();

    auto right = this->parseExpression(precedence);

    return std::make_shared<ast::expression::InfixExpression>(token, std::move(left), op, right);
}

std::shared_ptr<ast::Expression> Parser::parseBoolean() {
    auto token = this->get_cur_token();
    return std::make_shared<ast::expression::BooleanLiteral>(token, token.get_literal() == "true");
}

std::shared_ptr<ast::Expression> Parser::parseGroupedExpression() {
    this->nextToken();

    auto expression = this->parseExpression(Precedence::Lowest);
    if (!this->expectPeek(tokenize::TokenType::RParen)) {
        return nullptr;
    }

    return expression;
}

std::shared_ptr<ast::Expression> Parser::parseIfExpression() {
    auto token = this->get_cur_token();

    if (!this->expectPeek(tokenize::TokenType::LParen)) {
        return nullptr;
    }

    this->nextToken();
    auto condition = this->parseExpression(Precedence::Lowest);

    if (!this->expectPeek(tokenize::TokenType::RParen)) {
        return nullptr;
    }

    if (!this->expectPeek(tokenize::TokenType::LBrace)) {
        return nullptr;
    }

    auto consequence = this->parseBlockStatement();

    std::shared_ptr<ast::Statement> alternative = nullptr;
    if (this->peekTokenIs(tokenize::TokenType::Else)) {
        this->nextToken();

        if (!this->expectPeek(tokenize::TokenType::LBrace)) {
            return nullptr;
        }

        alternative = this->parseBlockStatement();
    }

    return std::make_shared<ast::expression::IfExpression>(token, std::move(condition), std::move(consequence), std::move(alternative));
}

std::shared_ptr<ast::Statement> Parser::parseBlockStatement() {
    auto token = this->get_cur_token();
    auto statements = std::vector<std::shared_ptr<ast::Statement>>();

    this->nextToken();

    while (!this->curTokenIs(tokenize::TokenType::RBrace) && !this->curTokenIs(tokenize::TokenType::EoF)) {
        auto statement = this->parseStatement();
        if (statement != nullptr) {
            statements.push_back(statement);
        }
        this->nextToken();
    }

    return std::make_shared<ast::statement::BlockStatement>(token, statements);
}

std::shared_ptr<ast::Expression> Parser::parseFunctionLiteral() {
    auto token = this->get_cur_token();

    if (!this->expectPeek(tokenize::TokenType::LParen)) {
        return nullptr;
    }

    auto parameters = this->parseFunctionParameters().value_or(std::vector<std::shared_ptr<ast::Expression>>());
    if (!this->expectPeek(tokenize::TokenType::LBrace)) {
        return nullptr;
    }

    auto body = this->parseBlockStatement();

    return std::make_shared<ast::expression::FunctionLiteral>(token, std::move(parameters), std::move(body));
}

std::optional<std::vector<std::shared_ptr<ast::Expression>>> Parser::parseFunctionParameters() {
    auto identifiers = std::vector<std::shared_ptr<ast::Expression>>();

    if (this->peekTokenIs(tokenize::TokenType::RParen)) {
        this->nextToken();
        return identifiers;
    }

    this->nextToken();

    auto firstArg = std::make_shared<ast::expression::IdentifierLiteral>(this->get_cur_token(), this->get_cur_token().get_literal());
    identifiers.push_back(firstArg);

    while (this->peekTokenIs(tokenize::TokenType::Comma)) {
        this->nextToken();
        this->nextToken();

        auto arg = std::make_shared<ast::expression::IdentifierLiteral>(this->get_cur_token(), this->get_cur_token().get_literal());
        identifiers.push_back(arg);
    }

    if (!this->expectPeek(tokenize::TokenType::RParen)) {
        return std::nullopt;
    }

    return identifiers;
}

std::shared_ptr<ast::Expression> Parser::parseCallExpression(std::shared_ptr<ast::Expression> function) {
    auto token = this->get_cur_token();
    auto arguments = this->parseExpressionList(tokenize::TokenType::RParen).value_or(std::vector<std::shared_ptr<ast::Expression>>());
    return std::make_shared<ast::expression::CallExpression>(token, std::move(function), arguments);
}

std::shared_ptr<ast::Expression> Parser::parseStringLiteral() {
    return std::make_shared<ast::expression::StringLiteral>(this->get_cur_token(), this->get_cur_token().get_literal());
}

std::shared_ptr<ast::Expression> Parser::parseArrayLiteral() {
    auto token = this->get_cur_token();
    auto elements = this->parseExpressionList(tokenize::TokenType::RBracket).value_or(std::vector<std::shared_ptr<ast::Expression>>());
    return std::make_shared<ast::expression::ArrayLiteral>(token, elements);
}

std::optional<std::vector<std::shared_ptr<ast::Expression>>> Parser::parseExpressionList(tokenize::TokenType end) {
    if (this->peekTokenIs(end)) {
        this->nextToken();
        return std::vector<std::shared_ptr<ast::Expression>>{};
    }

    this->nextToken();
    auto list = std::vector<std::shared_ptr<ast::Expression>>{this->parseExpression(Precedence::Lowest)};
    while (this->peekTokenIs(tokenize::TokenType::Comma)) {
        this->nextToken();
        this->nextToken();
        list.push_back(this->parseExpression(Precedence::Lowest));
    }

    if (!this->expectPeek(end)) {
        return std::nullopt;
    }

    return list;
}

std::shared_ptr<ast::Expression> Parser::parseIndexExpression(std::shared_ptr<ast::Expression> left) {
    auto token = this->get_cur_token();
    this->nextToken();

    auto index = this->parseExpression(Precedence::Lowest);
    if (!this->expectPeek(tokenize::TokenType::RBracket)) {
        return nullptr;
    }

    return std::make_shared<ast::expression::IndexExpression>(token, std::move(left), index);
}

} // namespace nugdev::compiler::parsing
