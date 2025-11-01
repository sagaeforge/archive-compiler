//
// Created by nugde on 25. 10. 10..
//

#pragma once

// #include "00_core/callable.h"
// #include "precedence.h"
// #include "ast/expression.h"
// #include "ast/statement.h"
//
// class block_statement;
// class PostfixExpression;
// class IndexExpression;
// class CallExpression;
// class InfixExpression;
// class FunctionExpression;
// class program;
// class IfExpression;
// class StringExpression;
// class NumberExpression;
// class ExpressionStatement;
// class TypeExpression;
// class VariableStatement;
// class return_statement;
// class IdentifierExpression;
//
// class parser final {
// public:
//  parser();
//
// public:
//  Node<program> parse(const std::vector<Token> &tokens);
//
// private:
//  Node<statement> parseStatement();
//
//  Node<block_statement> parseBlockStatement();
//
//  Node<return_statement> parseReturnStatement();
//
//  Node<VariableStatement> parseVariableStatement();
//
//  Node<ExpressionStatement> parseExpressionStatement();
//
//  Node<expression> parseExpression(precedence precedence = precedence::Lowest);
//
//  Node<IdentifierExpression> parseIdentifierExpression();
//
//  Node<TypeExpression> parseTypeExpression();
//
//  Node<NumberExpression> parseNumberExpression();
//
//  Node<StringExpression> parseStringExpression();
//
//  Node<expression> parseGroupedExpression();
//
//  Node<IfExpression> parseIfExpression();
//
//  Node<FunctionExpression> parseFunctionExpression();
//
//  Node<InfixExpression> parseInfixExpression(Node<expression> left);
//
//  Node<CallExpression> parseCallExpression(Node<expression> callee);
//
//  Node<IndexExpression> parseIndexExpression(Node<expression> left);
//
//  Node<PostfixExpression> parsePostfixExpression(Node<expression> left);
//
//  Node<expression> parseLabelExpression(Node<expression> left);
//
// private:
//  void nextToken();
//
//  Token peek() const;
//
//  Token currentToken() const;
//
// private:
//  std::vector<Token> m_tokens;
//  size_t m_index;
//
//  std::unordered_map<TokenType, std::function<Node<expression>()> > m_prefixParseFns;
//  std::unordered_map<TokenType, std::function<Node<expression>(Node<expression>)> > m_infixParseFns;
// };
