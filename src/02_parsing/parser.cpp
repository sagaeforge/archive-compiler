//
// Created by nugde on 25. 10. 10..
//

#include "parser.h"

#include "precedence.h"
// #include "ast/expression/boolean/BooleanExpression.h"
// #include "ast/expression/call/CallExpression.h"
// #include "ast/expression/function/FunctionExpression.h"
// #include "ast/expression/identifier/IdentifierExpression.h"
// #include "ast/expression/if/IfExpression.h"
// #include "ast/expression/infix/InfixExpression.h"
// #include "ast/expression/number/NumberExpression.h"
// #include "ast/expression/string/StringExpression.h"
// #include "ast/expression/type/TypeExpression.h"
// #include "ast/module/program.h"
// #include "ast/statement/block/block_statement.h"
// #include "ast/statement/expression/ExpressionStatement.h"
// #include "ast/statement/return/return_statement.h"
// #include "ast/statement/variable/VariableStatement.h"

// parser::parser() : m_tokens(), m_index(0) {
//     m_prefixParseFns = {
//         {TokenType::Identifier, [&]() { return this->parseIdentifierExpression(); }},
//         {TokenType::Number, [&]() { return this->parseNumberExpression(); }},
//         {TokenType::String, [&]() { return this->parseStringExpression(); }},
//         {TokenType::Plus, [&]() { return this->parsePrefixExpression(); }},
//         {TokenType::Minus, [&]() { return this->parsePrefixExpression(); }},
//         {TokenType::If, [&]() { return this->parseIfExpression(); }},
//         {TokenType::Function, [&]() { return this->parseFunctionExpression(); }},
//     };
//
//     m_infixParseFns = {
//         {TokenType::Plus, [&](Node<expression> left) { return this->parseInfixExpression(std::move(left)); }},
//         {TokenType::Minus, [&](Node<expression> left) { return this->parseInfixExpression(std::move(left)); }},
//         {TokenType::Divide, [&](Node<expression> left) { return this->parseInfixExpression(std::move(left)); }},
//         {TokenType::Multiply, [&](Node<expression> left) { return this->parseInfixExpression(std::move(left)); }},
//         {TokenType::Equal, [&](Node<expression> left) { return this->parseInfixExpression(std::move(left)); }},
//         {TokenType::LessEqual, [&](Node<expression> left) { return this->parseInfixExpression(std::move(left)); }},
//         {TokenType::GreaterEqual, [&](Node<expression> left) { return this->parseInfixExpression(std::move(left)); }},
//         {TokenType::Greater, [&](Node<expression> left) { return this->parseInfixExpression(std::move(left)); }},
//         {TokenType::Less, [&](Node<expression> left) { return this->parseInfixExpression(std::move(left)); }},
//
//         {TokenType::LeftParenthesis, [&](Node<expression> left) { return this->parseCallExpression(std::move(left)); }},
//     };
// }
//
// Node<program> parser::parse(const std::vector<Token> &tokens) {
//     m_tokens = tokens;
//     m_index = 0;
//
//     std::vector<Node<statement> > statements;
//     while (currentToken().getType() != TokenType::Illegal && m_index < m_tokens.size()) {
//         statements.push_back(parseStatement());
//     }
//
//     return std::make_shared<program>(std::make_shared<block_statement>(statements));
// }
//
// Node<program> parser::run(const std::vector<Token> &tokens) {
//     return parse(tokens);
// }
//
// Node<statement> parser::parseStatement() {
//     switch (const auto token = currentToken(); token.getType()) {
//         case TokenType::Break:
//             return parseBreakStatement();
//         case TokenType::Continue:
//             return parseContinueStatement();
//         case TokenType::Return:
//             return parseReturnStatement();
//         case TokenType::Value:
//             return parseValueStatement();
//         case TokenType::Variable:
//             return parseVariableStatement();
//         default:
//             return parseExpressionStatement();
//     }
// }
//
// Node<block_statement> parser::parseBlockStatement() {
//     // current: Token('{' | ':')
//     auto token = currentToken();
//     nextToken();
//
//     std::vector<Node<statement> > statements;
//     if (token.getType() == TokenType::LeftBrace) {
//         while (currentToken().getType() != TokenType::RightBrace && m_index < m_tokens.size()) {
//             statements.push_back(parseStatement());
//         }
//
//         if (token.getType() == TokenType::LeftBrace && currentToken().getType() == TokenType::RightBrace) {
//             nextToken();
//         }
//     } else {
//         statements.push_back(parseStatement());
//     }
//
//     return std::make_shared<block_statement>(statements);
// }
//
// Node<BreakStatement> parser::parseBreakStatement() {
//     // current: Token(Break)
//     auto token = currentToken();
//     nextToken();
//
//     // label 파싱
//     Node<IdentifierExpression> label = nullptr;
//     if (currentToken().getType() == TokenType::At) {
//         nextToken(); // current: Token(@)
//
//         // 식별자가 나와야 함.
//         label = parseIdentifierExpression();
//     }
//
//     return std::make_shared<BreakStatement>(token, label);
// }
//
// Node<ContinueStatement> parser::parseContinueStatement() {
//     // current: Token(Continue)
//     auto token = currentToken();
//     nextToken();
//
//     // label 파싱
//     Node<IdentifierExpression> label = nullptr;
//     if (currentToken().getType() == TokenType::At) {
//         nextToken(); // current: Token(@)
//
//         label = parseIdentifierExpression();
//     }
//
//     return std::make_shared<ContinueStatement>(token, label);
// }
//
// Node<return_statement> parser::parseReturnStatement() {
//     // current: Token(Return)
//     auto token = currentToken();
//     nextToken();
//
//     // label 파싱
//     Node<IdentifierExpression> label = nullptr;
//     if (currentToken().getType() == TokenType::At) {
//         nextToken(); // current: Token(@)
//
//         label = parseIdentifierExpression();
//     }
//
//     // value 파싱
//     Node<expression> value = nullptr;
//     try {
//         value = parseExpression();
//     } catch (...) {
//         /* Nothing */
//     }
//
//     return std::make_shared<return_statement>(token, label, value);
// }
//
// Node<ValueStatement> parser::parseValueStatement() {
//     // current: Token(val)
//     auto token = currentToken();
//     nextToken();
//
//     // identifier 파싱
//     Node<IdentifierExpression> name = parseIdentifierExpression();
//
//     Node<TypeExpression> type = nullptr;
//     if (currentToken().getType() == TokenType::Colon) {
//         nextToken(); // current: Token(:)
//         type = parseTypeExpression();
//     }
//
//     Node<expression> value = nullptr;
//     if (currentToken().getType() == TokenType::Assign) {
//         nextToken(); // current: Token(=)
//         value = parseExpression();
//     }
//
//     return std::make_shared<ValueStatement>(token, name, type, value);
// }
//
// Node<VariableStatement> parser::parseVariableStatement() {
//     // current: Token(val)
//     auto token = currentToken();
//     nextToken();
//
//     // identifier 파싱
//     Node<IdentifierExpression> name = parseIdentifierExpression();
//
//     Node<TypeExpression> type = nullptr;
//     if (currentToken().getType() == TokenType::Colon) {
//         nextToken(); // current: Token(:)
//         type = parseTypeExpression();
//     }
//
//     Node<expression> value = nullptr;
//     if (currentToken().getType() == TokenType::Assign) {
//         nextToken(); // current: Token(=)
//         value = parseExpression();
//     }
//
//     return std::make_shared<VariableStatement>(token, name, type, value);
// }
//
// Node<ForExpression> parser::parseForExpression() {
//     // current: Token(for)
//     auto token = currentToken();
//     nextToken();
//
//     Node<statement> init = nullptr;
//     Node<ExpressionStatement> condition = nullptr;
//     Node<ExpressionStatement> post = nullptr;
//     if (currentToken().getType() == TokenType::LeftParen) {
//         nextToken(); // current: Token('(')
//
//         // current: init?
//         try {
//             init = parseStatement();
//
//             if (currentToken().getType() != TokenType::SemiColon) {
//                 throw std::runtime_error("Expected ';' after for init.");
//             }
//             nextToken();
//         } catch (...) {
//             /* Nothing */
//         }
//
//         // current: condition!
//         condition = parseExpressionStatement();
//
//         // current: post?
//         if (currentToken().getType() == TokenType::SemiColon) {
//             nextToken(); // current: Token(';')
//
//             post = parseExpressionStatement();
//         }
//
//         if (currentToken().getType() != TokenType::RightParen) {
//             throw std::runtime_error("Expected ')' after for condition.");
//         }
//         nextToken();
//     }
//
//     // current: Token('{' | ':')
//     auto body = parseBlockStatement();
//
//     return std::make_shared<ForExpression>(token, nullptr, init, condition, post, body);
// }
//
// Node<ExpressionStatement> parser::parseExpressionStatement() {
//     return std::make_shared<ExpressionStatement>(parseExpression());
// }
//
// Node<expression> parser::parseExpression(const precedence precedence) {
//     const auto prefix = m_prefixParseFns.find(currentToken().getType());
//     if (prefix == m_prefixParseFns.end()) {
//         throw std::runtime_error("Invalid prefix parse function for token: " + currentToken().getLiteral());
//     }
//
//     auto leftNode = prefix->second();
//     while (precedence < getPrecedence(currentToken().getType())) {
//         auto infix = m_infixParseFns.find(currentToken().getType());
//         if (infix == m_infixParseFns.end()) {
//             return leftNode;
//         }
//
//         const auto rightNode = infix->second(leftNode);
//         leftNode = rightNode;
//     }
//
//     return leftNode;
// }
//
// Node<IdentifierExpression> parser::parseIdentifierExpression() {
//     // current: Token(식별자)
//     auto token = currentToken();
//     nextToken();
//
//     return std::make_shared<IdentifierExpression>(token);
// }
//
// Node<TypeExpression> parser::parseTypeExpression() {
//     // current: Token(식별자)
//     auto token = currentToken();
//     nextToken();
//
//     return std::make_shared<TypeExpression>(token);
// }
//
// Node<NumberExpression> parser::parseNumberExpression() {
//     auto token = currentToken();
//     nextToken();
//
//     return std::make_shared<NumberExpression>(token);
// }
//
// Node<StringExpression> parser::parseStringExpression() {
//     auto token = currentToken();
//     nextToken();
//
//     return std::make_shared<StringExpression>(token);
// }
//
// Node<PrefixExpression> parser::parsePrefixExpression() {
//     // current: Token('-' | '!')
//     auto token = currentToken();
//     nextToken();
//
//     auto right = parseExpression(getPrecedence(currentToken().getType()));
//     return std::make_shared<PrefixExpression>(token, right);
// }
//
// Node<BooleanExpression> parser::parseBooleanExpression() {
//     // current: Token(true | false)
//     auto token = currentToken();
//     nextToken();
//
//     return std::make_shared<BooleanExpression>(token);
// }
//
// Node<expression> parser::parseGroupedExpression() {
//     // current: Token('(')
//     nextToken();
//
//     auto expression = parseExpression();
//
//     // current: Token(')')
//     if (currentToken().getType() != TokenType::RightParen) {
//         throw std::runtime_error("Expected ')' after expression.");
//     }
//     nextToken();
//
//     return expression;
// }
//
// Node<IfExpression> parser::parseIfExpression() {
//     // current: Token(if || elif)
//     auto token = currentToken();
//     nextToken();
//
//     Node<expression> condition = nullptr;
//     if (currentToken().getType() != TokenType::LeftBrace && currentToken().getType() != TokenType::Colon) {
//         condition = parseExpression();
//     }
//
//     Node<block_statement> consequence = parseBlockStatement();
//
//     Node<IfExpression> otherBranch = nullptr;
//     if (currentToken().getType() == TokenType::Elif) {
//         otherBranch = parseIfExpression();
//     }
//
//     Node<block_statement> alternative = nullptr;
//     if (currentToken().getType() == TokenType::Else) {
//         alternative = parseBlockStatement();
//     }
//
//     return std::make_shared<IfExpression>(token, condition, consequence, otherBranch, alternative);
// }
//
// Node<FunctionExpression> parser::parseFunctionExpression() {
//     // current: Token(function)
//     const auto token = currentToken();
//     nextToken();
//
//     const auto name = parseIdentifierExpression();
//
//     if (currentToken().getType() != TokenType::LeftParen) {
//         throw std::runtime_error("Expected '(' after function name.");
//     }
//
//     // 파라미터 파싱
//     std::vector<Node<statement> > parameters;
//     do {
//         nextToken();
//
//         if (currentToken().getType() == TokenType::RightParen) {
//             break;
//         }
//
//         auto statement = parseStatement();
//         parameters.push_back(statement);
//     } while (currentToken().getType() == TokenType::Comma);
//
//     // current: Token(')')
//     if (currentToken().getType() != TokenType::RightParen) {
//         throw std::runtime_error("Expected ')' after function parameters.");
//     }
//     nextToken();
//
//     // 타입 지정
//     Node<TypeExpression> type = TypeExpression::NullType();
//     if (currentToken().getType() == TokenType::Colon) {
//         nextToken(); // current: Token(':')
//         type = parseTypeExpression();
//     }
//
//     // current: Token('{' | ':')
//     auto body = parseBlockStatement();
//     return std::make_shared<FunctionExpression>(token, name, body, type, parameters);
// }
//
// Node<ArrayExpression> parser::parseArrayExpression() {
//     // current: Token('[')
//     auto token = currentToken();
//
//     std::vector<Node<expression> > elements;
//     do {
//         nextToken();
//
//         if (currentToken().getType() == TokenType::RightBracket) {
//             break;
//         }
//
//         elements.push_back(parseExpression());
//     } while (currentToken().getType() == TokenType::Comma);
//
//     // current: Token(']')
//     if (currentToken().getType() != TokenType::RightBracket) {
//         throw std::runtime_error("Expected ']' after array elements.");
//     }
//     nextToken();
//
//     return std::make_shared<ArrayExpression>(token, elements);
// }
//
// Node<WhenExpression> parser::parseWhenExpression() {
//     // current: Token(when)
//     auto token = currentToken();
//     nextToken();
//
//     Node<expression> target = nullptr;
//     if (currentToken().getType() == TokenType::LeftParen) {
//         nextToken(); // current: Token('(')
//
//         target = parseExpression();
//
//         if (currentToken().getType() != TokenType::RightParen) {
//             throw std::runtime_error("Expected ')' after when target.");
//         }
//         nextToken();
//     }
//
//     // 중계식
//     if (currentToken().getType() != TokenType::LeftBrace) {
//         throw std::runtime_error("Expected '{' after when target.");
//     }
//     nextToken();
//
//     bool isAlternative = false;
//     Node<statement> alternative = nullptr;
//     std::vector<WhenBranch> branches;
//     do {
//         WhenBranch branch;
//         if (currentToken().getType() == TokenType::In) {
//             auto operand = currentToken();
//             if (target == nullptr) {
//                 throw std::runtime_error("Expected operand after 'in' keyword.");
//             }
//             nextToken();
//
//             auto expression = parseExpression();
//             branch.m_condition = std::make_shared<InfixExpression>(operand, target, expression);
//         } else if (currentToken().getType() == TokenType::Else) {
//             if (isAlternative) {
//                 throw std::runtime_error("Unexpected 'else' keyword.");
//             }
//             isAlternative = true;
//             nextToken();
//         } else {
//             auto expression = parseExpression();
//
//             if (target == nullptr) {
//                 branch.m_condition = expression;
//             } else {
//                 branch.m_condition =
//                         std::make_shared<InfixExpression>(Token(TokenType::Equal, "=="), target, expression);
//             }
//         }
//
//         if (currentToken().getType() != TokenType::LeftArrow) {
//             throw std::runtime_error("Expected '->' after when condition.");
//         }
//         nextToken();
//
//         if (!isAlternative) {
//             if (currentToken().getType() == TokenType::LeftBrace) {
//                 branch.m_body = parseBlockStatement();
//             } else {
//                 branch.m_body = parseExpressionStatement();
//             }
//         } else {
//             if (currentToken().getType() == TokenType::LeftBrace) {
//                 alternative = parseBlockStatement();
//             } else {
//                 alternative = parseExpressionStatement();
//             }
//         }
//     } while (currentToken().getType() == TokenType::RightBrace);
//     if (currentToken().getType() != TokenType::RightBrace) {
//         throw std::runtime_error("Expected '}' after when branches.");
//     }
//     nextToken();
//
//     return std::make_shared<WhenExpression>(token, branches, alternative);
// }
//
// Node<InfixExpression> parser::parseInfixExpression(Node<expression> left) {
//     // current: '+' | '-' | '*' | '/' | '%' | '==' | '!=' | '<' | '>' | '<=' | '>=' | 'in'
//     const auto token = currentToken();
//     const auto precedence = getPrecedence(token.getType());
//     nextToken();
//
//     auto right = parseExpression(precedence);
//     return std::make_shared<InfixExpression>(token, left, right);
// }
//
// Node<CallExpression> parser::parseCallExpression(Node<expression> callee) {
//     // current: '('
//     auto token = currentToken();
//
//     std::vector<Node<expression> > list;
//     do {
//         nextToken();
//
//         if (currentToken().getType() == TokenType::RightParen) {
//             break;
//         }
//
//         auto expression = parseExpression();
//         list.push_back(expression);
//     } while (currentToken().getType() == TokenType::Comma);
//
//     if (currentToken().getType() != TokenType::RightParen) {
//         throw std::runtime_error("Expected ')' after call arguments.");
//     }
//     nextToken();
//
//     return std::make_shared<CallExpression>(token, callee, list);
// }
//
// Node<IndexExpression> parser::parseIndexExpression(Node<expression> left) {
//     // current: '['
//     auto token = currentToken();
//     nextToken();
//
//     auto index = parseExpression();
//     if (currentToken().getType() != TokenType::RightBracket) {
//         throw std::runtime_error("Expected ']' after index expression.");
//     }
//     nextToken();
//
//     return std::make_shared<IndexExpression>(token, left, index);
// }
//
// Node<PostfixExpression> parser::parsePostfixExpression(Node<expression> left) {
//     // current: '++' | '--'
//     auto token = currentToken();
//     nextToken();
//
//     return std::make_shared<PostfixExpression>(token, left);
// }
//
// Node<expression> parser::parseLabelExpression(Node<expression> left) {
//     // left는 identifier만 지원함.
//     if (!left->is<IdentifierExpression>()) {
//         throw std::runtime_error("LabelExpression is only support identifier left expression");
//     }
//
//     // current: '@'
//     auto token = currentToken();
//     nextToken();
//
//     auto right = parseExpression();
//     if (!right->is<ForExpression>()) {
//         throw std::runtime_error("LabelExpression is only support for expression right expression");
//     }
//
//     auto rightNode = right->as<ForExpression>();
//     return rightNode->setLabel(left->as<IdentifierExpression>());
// }
//
// void parser::nextToken() {
//     ++this->m_index;
// }
//
// Token parser::peek() const {
//     if (m_index + 1 >= m_tokens.size())
//         return {};
//     return m_tokens[m_index + 1];
// }
//
// Token parser::currentToken() const {
//     if (m_index >= m_tokens.size())
//         return {};
//     return m_tokens[m_index];
// }
