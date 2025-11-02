//
// Created by nugde on 25. 10. 10..
//

#include "parser.h"

#include "precedence.h"
#include "ast/module/program.h"
#include "ast/expression/call_expression.h"
#include "ast/expression/function_expression.h"
#include "ast/expression/identifier_expression.h"
#include "ast/expression/if_expression.h"
#include "ast/expression/infix_expression.h"
#include "ast/expression/number_expression.h"
#include "ast/expression/prefix_expression.h"
#include "ast/expression/string_expression.h"
#include "ast/expression/type_expression.h"
#include "ast/module/program.h"
#include "ast/statement/block_statement.h"
#include "ast/statement/expression_statement.h"
#include "ast/statement/return_statement.h"
#include "ast/statement/variable_statement.h"

Parser::Parser(const std::vector<Token> &tokens) : m_stream(tokens), m_index(0) {
    m_prefixParseFns = {
        {TokenType::Identifier, [&]() { return this->parseIdentifierExpression(); }},
        {TokenType::Number, [&]() { return this->parseNumberExpression(); }},
        {TokenType::String, [&]() { return this->parseStringExpression(); }},
        {TokenType::Minus, [&]() { return this->parsePrefixExpression(); }},
        {TokenType::LeftParenthesis, [&]() { return this->parseGroupedExpression(); }},
        {TokenType::If, [&]() { return this->parseIfExpression(); }},
        {TokenType::Function, [&]() { return this->parseFunctionExpression(); }},
    };

    m_infixParseFns = {
        {TokenType::Plus, [&](const Node<Expression> &left) { return this->parseInfixExpression(left); }},
        {TokenType::Minus, [&](const Node<Expression> &left) { return this->parseInfixExpression(left); }},
        {TokenType::Multiply, [&](const Node<Expression> &left) { return this->parseInfixExpression(left); }},
        {TokenType::Divide, [&](const Node<Expression> &left) { return this->parseInfixExpression(left); }},
        {TokenType::Equal, [&](const Node<Expression> &left) { return this->parseInfixExpression(left); }},
        {TokenType::Less, [&](const Node<Expression> &left) { return this->parseInfixExpression(left); }},
        {TokenType::LessEqual, [&](const Node<Expression> &left) { return this->parseInfixExpression(left); }},
        {TokenType::Greater, [&](const Node<Expression> &left) { return this->parseInfixExpression(left); }},
        {TokenType::GreaterEqual, [&](const Node<Expression> &left) { return this->parseInfixExpression(left); }},
        {TokenType::Assign, [&](const Node<Expression> &left) { return this->parseInfixExpression(left); }},

        {TokenType::LeftParenthesis, [&](Node<Expression> left) { return this->parseCallExpression(std::move(left)); }},
    };
}

Node<Module> Parser::parse() {
    std::vector<Node<Statement> > statements;
    while (currentToken().type() != TokenType::Illegal && m_index < m_stream.size()) {
        statements.push_back(parseStatement());
    }

    return std::make_shared<Program>(statements);
}

Node<Statement> Parser::parseStatement() {
    switch (const auto token = currentToken(); token.type()) {
        case TokenType::Return:
            return this->parseReturnStatement();
        case TokenType::Variable:
            return parseVariableStatement();
        default:
            return parseExpressionStatement();
    }
}

Node<BlockStatement> Parser::parseBlockStatement() {
    // current: Token('{' | ':')
    auto token = currentToken();
    nextToken();

    std::vector<Node<Statement> > statements;
    if (token.type() == TokenType::LeftBrace) {
        while (currentToken().type() != TokenType::RightBrace && m_index < m_stream.size()) {
            statements.push_back(parseStatement());
        }

        if (token.type() == TokenType::LeftBrace && currentToken().type() == TokenType::RightBrace) {
            nextToken();
        }
    } else {
        statements.push_back(parseStatement());
    }

    return std::make_shared<BlockStatement>(statements);
}

Node<ReturnStatement> Parser::parseReturnStatement() {
    // current: Token(Return)
    auto token = currentToken();
    nextToken();

    // value 파싱
    Node<Expression> value = nullptr;
    try {
        value = parseExpression();
    } catch (...) {
        /* Nothing */
    }

    return std::make_shared<ReturnStatement>(token, value);
}

Node<VariableStatement> Parser::parseVariableStatement() {
    // current: Token(val)
    auto token = currentToken();
    nextToken();

    // identifier 파싱
    Node<IdentifierExpression> name = parseIdentifierExpression();

    Node<TypeExpression> type = nullptr;
    if (currentToken().type() == TokenType::Colon) {
        nextToken(); // current: Token(:)
        type = parseTypeExpression();
    }

    Node<Expression> value = nullptr;
    if (currentToken().type() == TokenType::Assign) {
        nextToken(); // current: Token(=)
        value = parseExpression();
    }

    return std::make_shared<VariableStatement>(token, name, type, value);
}

Node<ExpressionStatement> Parser::parseExpressionStatement() {
    return std::make_shared<ExpressionStatement>(parseExpression());
}

Node<Expression> Parser::parseExpression(precedence precedence) {
    const auto prefix = m_prefixParseFns.find(currentToken().type());
    if (prefix == m_prefixParseFns.end()) {
        throw std::runtime_error("Invalid prefix parse function for token: " + currentToken().literal());
    }

    auto leftNode = prefix->second();
    while (precedence < getPrecedence(currentToken().type())) {
        auto infix = m_infixParseFns.find(currentToken().type());
        if (infix == m_infixParseFns.end()) {
            return leftNode;
        }

        const auto rightNode = infix->second(leftNode);
        leftNode = rightNode;
    }

    return leftNode;
}

Node<IdentifierExpression> Parser::parseIdentifierExpression() {
    // current: Token(식별자)
    auto token = currentToken();
    nextToken();

    return std::make_shared<IdentifierExpression>(token);
}

Node<TypeExpression> Parser::parseTypeExpression() {
    // current: Token(식별자)
    auto token = currentToken();
    nextToken();

    return std::make_shared<TypeExpression>(token);
}

Node<NumberExpression> Parser::parseNumberExpression() {
    auto token = currentToken();
    nextToken();

    return std::make_shared<NumberExpression>(token);
}

Node<StringExpression> Parser::parseStringExpression() {
    auto token = currentToken();
    nextToken();

    return std::make_shared<StringExpression>(token);
}

Node<Expression> Parser::parseGroupedExpression() {
    // current: Token('(')
    nextToken();

    auto expression = parseExpression();

    // current: Token(')')
    if (currentToken().type() != TokenType::RightParenthesis) {
        throw std::runtime_error("Expected ')' after expression.");
    }
    nextToken();

    return expression;
}

Node<IfExpression> Parser::parseIfExpression() {
    // current: Token(if || elif)
    auto token = currentToken();
    nextToken();

    Node<Expression> condition = nullptr;
    if (currentToken().type() != TokenType::LeftBrace && currentToken().type() != TokenType::Colon) {
        condition = parseExpression();
    }

    Node<BlockStatement> consequence = parseBlockStatement();

    Node<IfExpression> otherBranch = nullptr;
    if (currentToken().type() == TokenType::Elif) {
        otherBranch = parseIfExpression();
    }

    Node<BlockStatement> alternative = nullptr;
    if (currentToken().type() == TokenType::Else) {
        alternative = parseBlockStatement();
    }

    return std::make_shared<IfExpression>(token, condition, consequence, otherBranch, alternative);
}

Node<FunctionExpression> Parser::parseFunctionExpression() {
    // current: Token(function)
    const auto token = currentToken();
    nextToken();

    const auto name = parseIdentifierExpression();

    if (currentToken().type() != TokenType::LeftParenthesis) {
        throw std::runtime_error("Expected '(' after function name.");
    }

    // 파라미터 파싱
    std::vector<Node<Statement> > parameters;
    do {
        nextToken();

        if (currentToken().type() == TokenType::RightParenthesis) {
            break;
        }

        auto statement = parseStatement();
        parameters.push_back(statement);
    } while (currentToken().type() == TokenType::Comma);

    // current: Token(')')
    if (currentToken().type() != TokenType::RightParenthesis) {
        throw std::runtime_error("Expected ')' after function parameters.");
    }
    nextToken();

    // 타입 지정
    Node<TypeExpression> type = nullptr;
    if (currentToken().type() == TokenType::Colon) {
        nextToken(); // current: Token(':')
        type = parseTypeExpression();
    }

    // current: Token('{' | ':')
    auto body = parseBlockStatement();
    return std::make_shared<FunctionExpression>(token, name, parameters, body, type);
}

Node<InfixExpression> Parser::parseInfixExpression(const Node<Expression> &left) {
    // current: '+' | '-' | '*' | '/' | '%' | '==' | '!=' | '<' | '>' | '<=' | '>=' | 'in'
    const auto token = currentToken();
    const auto precedence = getPrecedence(token.type());
    nextToken();

    auto right = parseExpression(precedence);
    return std::make_shared<InfixExpression>(token, left, right);
}

Node<CallExpression> Parser::parseCallExpression(Node<Expression> callee) {
    // current: '('
    auto token = currentToken();

    std::vector<Node<Expression> > list;
    do {
        nextToken();

        if (currentToken().type() == TokenType::RightParenthesis) {
            break;
        }

        auto expression = parseExpression();
        list.push_back(expression);
    } while (currentToken().type() == TokenType::Comma);

    if (currentToken().type() != TokenType::RightParenthesis) {
        throw std::runtime_error("Expected ')' after call arguments.");
    }
    nextToken();

    return std::make_shared<CallExpression>(token, callee, list);
}

Node<PrefixExpression> Parser::parsePrefixExpression() {
    // current: Token('-' | '!')
    auto token = currentToken();
    nextToken();

    auto right = parseExpression(getPrecedence(currentToken().type()));
    return std::make_shared<PrefixExpression>(token, right);
}


void Parser::nextToken() {
    ++this->m_index;
}

Token Parser::peek() const {
    if (m_index + 1 >= m_stream.size())
        return Token::illegal();
    return m_stream[m_index + 1];
}

Token Parser::currentToken() const {
    if (m_index >= m_stream.size())
        return Token::illegal();
    return m_stream[m_index];
}

