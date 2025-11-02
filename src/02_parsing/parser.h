//
// Created by nugde on 25. 10. 10..
//

#pragma once

#include "precedence.h"
#include "02_parsing/ast/node.h"

class BlockStatement;
class CallExpression;
class InfixExpression;
class FunctionExpression;
class PrefixExpression;
class Program;
class IfExpression;
class StringExpression;
class NumberExpression;
class ExpressionStatement;
class TypeExpression;
class VariableStatement;
class ReturnStatement;
class IdentifierExpression;

class Parser final {
public:
    explicit Parser(const std::vector<Token> &tokens);
    ~Parser() = default;

public:
    Node<Module> parse();

private:
    Node<Statement> parseStatement();
    Node<BlockStatement> parseBlockStatement();
    Node<ReturnStatement> parseReturnStatement();
    Node<VariableStatement> parseVariableStatement();
    Node<ExpressionStatement> parseExpressionStatement();
    Node<Expression> parseExpression(precedence precedence = precedence::Lowest);
    Node<IdentifierExpression> parseIdentifierExpression();
    Node<TypeExpression> parseTypeExpression();
    Node<NumberExpression> parseNumberExpression();
    Node<StringExpression> parseStringExpression();
    Node<Expression> parseGroupedExpression();
    Node<IfExpression> parseIfExpression();
    Node<FunctionExpression> parseFunctionExpression();
    Node<InfixExpression> parseInfixExpression(const Node<Expression> &left);
    Node<CallExpression> parseCallExpression(Node<Expression> callee);
    Node<PrefixExpression> parsePrefixExpression();

private:
    void nextToken();
    Token peek() const;
    Token currentToken() const;

private:
    std::vector<Token> m_stream;
    size_t m_index{};
    std::unordered_map<TokenType, std::function<Node<Expression>()> > m_prefixParseFns;
    std::unordered_map<TokenType, std::function<Node<Expression>(Node<Expression>)> > m_infixParseFns;
};
