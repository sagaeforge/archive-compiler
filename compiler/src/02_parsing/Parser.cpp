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

Precedence get_precedence(tokenize::TokenType type) {
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

Parser::Parser(const TokenStream &tokens) : stream(tokens), errors() {
    prefixParseFns = {
        {tokenize::TokenType::Ident, [&]() { return this->parse_identifier(); }},
        {tokenize::TokenType::Number, [&]() { return this->parse_number_literal(); }},
        {tokenize::TokenType::String, [&]() { return this->parse_string_literal(); }},
        {tokenize::TokenType::ExclamationMark, [&]() { return this->parse_prefix_expression(); }},
        {tokenize::TokenType::Minus, [&]() { return this->parse_prefix_expression(); }},
        {tokenize::TokenType::True, [&]() { return this->parse_boolean(); }},
        {tokenize::TokenType::False, [&]() { return this->parse_boolean(); }},
        {tokenize::TokenType::LParen, [&]() { return this->parse_grouped_expression(); }},
        {tokenize::TokenType::If, [&]() { return this->parse_if_expression(); }},
        {tokenize::TokenType::Function, [&]() { return this->parse_function_literal(); }},
        {tokenize::TokenType::LBracket, [&]() { return this->parse_array_literal(); }},
    };

    infixParseFns = {
        {tokenize::TokenType::Plus, [&](std::shared_ptr<ast::Expression> expression) { return this->parse_infix_expression(std::move(expression)); }},
        {tokenize::TokenType::Minus, [&](std::shared_ptr<ast::Expression> expression) { return this->parse_infix_expression(std::move(expression)); }},
        {tokenize::TokenType::Slash, [&](std::shared_ptr<ast::Expression> expression) { return this->parse_infix_expression(std::move(expression)); }},
        {tokenize::TokenType::Asterisk, [&](std::shared_ptr<ast::Expression> expression) { return this->parse_infix_expression(std::move(expression)); }},
        {tokenize::TokenType::Equal, [&](std::shared_ptr<ast::Expression> expression) { return this->parse_infix_expression(std::move(expression)); }},
        {tokenize::TokenType::NotEqual, [&](std::shared_ptr<ast::Expression> expression) { return this->parse_infix_expression(std::move(expression)); }},
        {tokenize::TokenType::LessThan, [&](std::shared_ptr<ast::Expression> expression) { return this->parse_infix_expression(std::move(expression)); }},
        {tokenize::TokenType::GreaterThan, [&](std::shared_ptr<ast::Expression> expression) { return this->parse_infix_expression(std::move(expression)); }},

        {tokenize::TokenType::LParen, [&](std::shared_ptr<ast::Expression> expression) { return this->parse_call_expression(std::move(expression)); }},
        {tokenize::TokenType::LBracket, [&](std::shared_ptr<ast::Expression> expression) { return this->parse_index_expression(std::move(expression)); }},
    };

    next_token();
    next_token();
}

tokenize::Token Parser::get_cur_token() const { return this->stream.checkpoint().value_or(tokenize::Token::empty()); }

tokenize::Token Parser::get_peek_token() const { return (++this->stream.checkpoint()).value_or(tokenize::Token::empty()); }

void Parser::next_token() {
    auto checkpoint = this->stream.checkpoint();
    this->stream.commit(checkpoint.next());
}

bool Parser::cur_token_is(tokenize::TokenType type) { return this->get_cur_token().get_type() == type; }

bool Parser::peek_token_is(tokenize::TokenType type) { return this->get_peek_token().get_type() == type; }

bool Parser::expect_peek(tokenize::TokenType type) {
    if (this->peek_token_is(type)) {
        this->next_token();
        return true;
    }

    this->errors.push_back(std::make_exception_ptr(ParserError()));
    return false;
}

std::vector<std::exception_ptr> Parser::get_errors() const { return this->errors; }

void Parser::no_prefix_parse_fn_error(tokenize::TokenType type) { this->errors.push_back(std::make_exception_ptr(ParserError())); }

Precedence Parser::peek_precedence() {
    auto checkpoint = this->stream.checkpoint();
    return get_precedence(checkpoint.value_or(tokenize::Token::empty()).get_type());
}

Precedence Parser::cur_precedence() {
    auto checkpoint = this->stream.checkpoint();
    return get_precedence(checkpoint.value_or(tokenize::Token::empty()).get_type());
}

std::shared_ptr<ast::Module> Parser::parse_program() {
    auto statements = std::vector<std::shared_ptr<ast::Statement>>();

    while (this->cur_token_is(tokenize::TokenType::EoF) == false) {
        auto statement = this->parse_statement();
        if (statement != nullptr) {
            statements.push_back(statement);
        }
        this->next_token();
    }

    return std::make_shared<ast::module::Program>(statements);
}

std::shared_ptr<ast::Statement> Parser::parse_statement() {
    switch (this->get_cur_token().get_type()) {
    case tokenize::TokenType::Let:
        return this->parse_let_statement();
    case tokenize::TokenType::Return:
        return this->parse_return_statement();
    case tokenize::TokenType::Break:
        return this->parse_break_statement();
    case tokenize::TokenType::Continue:
        return this->parse_continue_statement();
    default:
        return this->parse_expression_statement();
    }
}

std::shared_ptr<ast::Statement> Parser::parse_let_statement() {
    auto token = this->get_cur_token();
    if (!this->expect_peek(tokenize::TokenType::Ident)) {
        return nullptr;
    }

    auto name = std::make_shared<ast::expression::IdentifierLiteral>(this->get_cur_token().get_literal());
    if (!this->expect_peek(tokenize::TokenType::Assign)) {
        return nullptr;
    }

    this->next_token();
    auto value = this->parse_expression(Precedence::Lowest);
    if (this->peek_token_is(tokenize::TokenType::SemiColon)) {
        this->next_token();
    }

    return std::make_shared<ast::statement::LetStatement>(name, token, value);
}

std::shared_ptr<ast::Statement> Parser::parse_return_statement() {
    auto token = this->get_cur_token();
    this->next_token();

    auto returnValue = this->parse_expression(Precedence::Lowest);
    if (this->peek_token_is(tokenize::TokenType::SemiColon)) {
        this->next_token();
    }

    return std::make_shared<ast::statement::ReturnStatement>(token, returnValue);
}

std::shared_ptr<ast::Statement> Parser::parse_break_statement() {
    auto token = this->get_cur_token();
    this->next_token();
    if (this->peek_token_is(tokenize::TokenType::SemiColon)) {
        this->next_token();
    }

    return std::make_shared<ast::statement::BreakStatement>(token);
}

std::shared_ptr<ast::Statement> Parser::parse_continue_statement() {
    auto token = this->get_cur_token();
    this->next_token();
    if (this->peek_token_is(tokenize::TokenType::SemiColon)) {
        this->next_token();
    }

    return std::make_shared<ast::statement::ContinueStatement>(token);
}

std::shared_ptr<ast::Statement> Parser::parse_expression_statement() {
    auto token = this->get_cur_token();
    auto expression = this->parse_expression(Precedence::Lowest);

    if (this->peek_token_is(tokenize::TokenType::SemiColon)) {
        this->next_token();
    }

    return std::make_shared<ast::statement::ExpressionStatement>(token, expression);
}

std::shared_ptr<ast::Expression> Parser::parse_expression(Precedence precedence) {
    auto prefix = this->prefixParseFns.find(this->get_cur_token().get_type());
    if (prefix == this->prefixParseFns.end()) {
        this->no_prefix_parse_fn_error(this->get_cur_token().get_type());
        return nullptr;
    }
    auto leftExp = prefix->second();

    while (!this->peek_token_is(tokenize::TokenType::SemiColon) && precedence < this->peek_precedence()) {
        auto infix = this->infixParseFns[this->get_peek_token().get_type()];
        if (infix == nullptr) {
            return leftExp;
        }

        this->next_token();

        leftExp = infix(std::move(leftExp));
    }

    return leftExp;
}

std::shared_ptr<ast::Expression> Parser::parse_identifier() {
    return std::make_shared<ast::expression::IdentifierLiteral>(this->get_cur_token().get_literal());
}

std::shared_ptr<ast::Expression> Parser::parse_number_literal() {
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

std::shared_ptr<ast::Expression> Parser::parse_prefix_expression() {
    auto token = this->get_cur_token();
    auto op = token.get_literal();

    this->next_token();

    auto right = this->parse_expression(Precedence::Prefix);
    return std::make_shared<ast::expression::PrefixExpression>(token, op, right);
}

std::shared_ptr<ast::Expression> Parser::parse_infix_expression(std::shared_ptr<ast::Expression> left) {
    auto token = this->get_cur_token();
    auto op = token.get_literal();
    auto precedence = this->cur_precedence();

    this->next_token();

    auto right = this->parse_expression(precedence);

    return std::make_shared<ast::expression::InfixExpression>(token, std::move(left), op, right);
}

std::shared_ptr<ast::Expression> Parser::parse_boolean() {
    auto token = this->get_cur_token();
    return std::make_shared<ast::expression::BooleanLiteral>(token, token.get_literal() == "true");
}

std::shared_ptr<ast::Expression> Parser::parse_grouped_expression() {
    this->next_token();

    auto expression = this->parse_expression(Precedence::Lowest);
    if (!this->expect_peek(tokenize::TokenType::RParen)) {
        return nullptr;
    }

    return expression;
}

std::shared_ptr<ast::Expression> Parser::parse_if_expression() {
    auto token = this->get_cur_token();

    if (!this->expect_peek(tokenize::TokenType::LParen)) {
        return nullptr;
    }

    this->next_token();
    auto condition = this->parse_expression(Precedence::Lowest);

    if (!this->expect_peek(tokenize::TokenType::RParen)) {
        return nullptr;
    }

    if (!this->expect_peek(tokenize::TokenType::LBrace)) {
        return nullptr;
    }

    auto consequence = this->parse_block_statement();

    std::shared_ptr<ast::Statement> alternative = nullptr;
    if (this->peek_token_is(tokenize::TokenType::Else)) {
        this->next_token();

        if (!this->expect_peek(tokenize::TokenType::LBrace)) {
            return nullptr;
        }

        alternative = this->parse_block_statement();
    }

    return std::make_shared<ast::expression::IfExpression>(token, std::move(condition), std::move(consequence), std::move(alternative));
}

std::shared_ptr<ast::Statement> Parser::parse_block_statement() {
    auto token = this->get_cur_token();
    auto statements = std::vector<std::shared_ptr<ast::Statement>>();

    this->next_token();

    while (!this->cur_token_is(tokenize::TokenType::RBrace) && !this->cur_token_is(tokenize::TokenType::EoF)) {
        auto statement = this->parse_statement();
        if (statement != nullptr) {
            statements.push_back(statement);
        }
        this->next_token();
    }

    return std::make_shared<ast::statement::BlockStatement>(token, statements);
}

std::shared_ptr<ast::Expression> Parser::parse_function_literal() {
    auto token = this->get_cur_token();

    if (!this->expect_peek(tokenize::TokenType::LParen)) {
        return nullptr;
    }

    auto parameters = this->parse_function_parameters().value_or(std::vector<std::shared_ptr<ast::Expression>>());
    if (!this->expect_peek(tokenize::TokenType::LBrace)) {
        return nullptr;
    }

    auto body = this->parse_block_statement();

    return std::make_shared<ast::expression::FunctionLiteral>(token, std::move(parameters), std::move(body));
}

std::optional<std::vector<std::shared_ptr<ast::Expression>>> Parser::parse_function_parameters() {
    auto identifiers = std::vector<std::shared_ptr<ast::Expression>>();

    if (this->peek_token_is(tokenize::TokenType::RParen)) {
        this->next_token();
        return identifiers;
    }

    this->next_token();

    auto firstArg = std::make_shared<ast::expression::IdentifierLiteral>(this->get_cur_token(), this->get_cur_token().get_literal());
    identifiers.push_back(firstArg);

    while (this->peek_token_is(tokenize::TokenType::Comma)) {
        this->next_token();
        this->next_token();

        auto arg = std::make_shared<ast::expression::IdentifierLiteral>(this->get_cur_token(), this->get_cur_token().get_literal());
        identifiers.push_back(arg);
    }

    if (!this->expect_peek(tokenize::TokenType::RParen)) {
        return std::nullopt;
    }

    return identifiers;
}

std::shared_ptr<ast::Expression> Parser::parse_call_expression(std::shared_ptr<ast::Expression> function) {
    auto token = this->get_cur_token();
    auto arguments = this->parse_expression_list(tokenize::TokenType::RParen).value_or(std::vector<std::shared_ptr<ast::Expression>>());
    return std::make_shared<ast::expression::CallExpression>(token, std::move(function), arguments);
}

std::shared_ptr<ast::Expression> Parser::parse_string_literal() {
    return std::make_shared<ast::expression::StringLiteral>(this->get_cur_token(), this->get_cur_token().get_literal());
}

std::shared_ptr<ast::Expression> Parser::parse_array_literal() {
    auto token = this->get_cur_token();
    auto elements = this->parse_expression_list(tokenize::TokenType::RBracket).value_or(std::vector<std::shared_ptr<ast::Expression>>());
    return std::make_shared<ast::expression::ArrayLiteral>(token, elements);
}

std::optional<std::vector<std::shared_ptr<ast::Expression>>> Parser::parse_expression_list(tokenize::TokenType end) {
    if (this->peek_token_is(end)) {
        this->next_token();
        return std::vector<std::shared_ptr<ast::Expression>>{};
    }

    this->next_token();
    auto list = std::vector<std::shared_ptr<ast::Expression>>{this->parse_expression(Precedence::Lowest)};
    while (this->peek_token_is(tokenize::TokenType::Comma)) {
        this->next_token();
        this->next_token();
        list.push_back(this->parse_expression(Precedence::Lowest));
    }

    if (!this->expect_peek(end)) {
        return std::nullopt;
    }

    return list;
}

std::shared_ptr<ast::Expression> Parser::parse_index_expression(std::shared_ptr<ast::Expression> left) {
    auto token = this->get_cur_token();
    this->next_token();

    auto index = this->parse_expression(Precedence::Lowest);
    if (!this->expect_peek(tokenize::TokenType::RBracket)) {
        return nullptr;
    }

    return std::make_shared<ast::expression::IndexExpression>(token, std::move(left), index);
}

} // namespace nugdev::compiler::parsing
