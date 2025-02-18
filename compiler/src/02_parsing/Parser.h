#pragma once

#include <exception>
#include <memory>
#include <unordered_map>
#include <vector>

#include "00_app/stream/Stream.hpp"
#include "01_tokenize/Token.h"
#include "02_parsing/ast/AST.h"

namespace nugdev::compiler {

namespace parsing {

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

Precedence getPrecedence(tokenize::TokenType type);
using prefixParseFn = std::function<std::shared_ptr<ast::Expression>()>;
using infixParseFn = std::function<std::shared_ptr<ast::Expression>(std::shared_ptr<ast::Expression>)>;

class ParserError : public std::exception {};

class Parser {
    using TokenStream = stream::Stream<tokenize::Token>;
    using TokenStreamIterator = typename TokenStream::const_iterator<tokenize::Token>;

  public:
    Parser(const TokenStream &tokens);

  public:
    tokenize::Token get_cur_token() const;
    tokenize::Token get_peek_token() const;
    void nextToken();
    bool curTokenIs(tokenize::TokenType type);
    bool peekTokenIs(tokenize::TokenType type);
    bool expectPeek(tokenize::TokenType type);
    std::vector<std::exception_ptr> getErrors() const;
    void noPrefixParseFnError(tokenize::TokenType type);
    Precedence peekPrecedence();
    Precedence curPrecedence();

  public: // parse
    std::shared_ptr<ast::Module> parseProgram();
    std::shared_ptr<ast::Statement> parseStatement();
    std::shared_ptr<ast::Statement> parseLetStatement();
    std::shared_ptr<ast::Statement> parseReturnStatement();
    std::shared_ptr<ast::Statement> parseBreakStatement();
    std::shared_ptr<ast::Statement> parseContinueStatement();
    std::shared_ptr<ast::Statement> parseExpressionStatement();
    std::shared_ptr<ast::Expression> parseExpression(const Precedence precedence);
    std::shared_ptr<ast::Expression> parseIdentifier();
    std::shared_ptr<ast::Expression> parseNumberLiteral();
    std::shared_ptr<ast::Expression> parsePrefixExpression();
    std::shared_ptr<ast::Expression> parseInfixExpression(std::shared_ptr<ast::Expression> left);
    std::shared_ptr<ast::Expression> parseBoolean();
    std::shared_ptr<ast::Expression> parseGroupedExpression();
    std::shared_ptr<ast::Expression> parseIfExpression();
    std::shared_ptr<ast::Statement> parseBlockStatement();
    std::shared_ptr<ast::Expression> parseFunctionLiteral();
    std::optional<std::vector<std::shared_ptr<ast::Expression>>> parseFunctionParameters();
    std::shared_ptr<ast::Expression> parseCallExpression(std::shared_ptr<ast::Expression> function);
    std::shared_ptr<ast::Expression> parseStringLiteral();
    std::shared_ptr<ast::Expression> parseArrayLiteral();
    std::optional<std::vector<std::shared_ptr<ast::Expression>>> parseExpressionList(tokenize::TokenType end);
    std::shared_ptr<ast::Expression> parseIndexExpression(std::shared_ptr<ast::Expression> left);

  private:
    TokenStream stream;
    std::unordered_map<tokenize::TokenType, prefixParseFn> prefixParseFns;
    std::unordered_map<tokenize::TokenType, infixParseFn> infixParseFns;
    std::vector<std::exception_ptr> errors;
};

} // namespace parsing
} // namespace nugdev::compiler