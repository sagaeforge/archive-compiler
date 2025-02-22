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

Precedence get_precedence(tokenize::TokenType type);
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
    void next_token();
    bool cur_token_is(tokenize::TokenType type);
    bool peek_token_is(tokenize::TokenType type);
    bool expect_peek(tokenize::TokenType type);
    std::vector<std::exception_ptr> get_errors() const;
    void no_prefix_parse_fn_error(tokenize::TokenType type);
    Precedence peek_precedence();
    Precedence cur_precedence();

  public: // parse
    std::shared_ptr<ast::Module> parse_program();
    std::shared_ptr<ast::Statement> parse_statement();
    std::shared_ptr<ast::Statement> parse_let_statement();
    std::shared_ptr<ast::Statement> parse_return_statement();
    std::shared_ptr<ast::Statement> parse_break_statement();
    std::shared_ptr<ast::Statement> parse_continue_statement();
    std::shared_ptr<ast::Statement> parse_expression_statement();
    std::shared_ptr<ast::Expression> parse_expression(const Precedence precedence);
    std::shared_ptr<ast::Expression> parse_identifier();
    std::shared_ptr<ast::Expression> parse_number_literal();
    std::shared_ptr<ast::Expression> parse_prefix_expression();
    std::shared_ptr<ast::Expression> parse_infix_expression(std::shared_ptr<ast::Expression> left);
    std::shared_ptr<ast::Expression> parse_boolean();
    std::shared_ptr<ast::Expression> parse_grouped_expression();
    std::shared_ptr<ast::Expression> parse_if_expression();
    std::shared_ptr<ast::Statement> parse_block_statement();
    std::shared_ptr<ast::Expression> parse_function_literal();
    std::optional<std::vector<std::shared_ptr<ast::Expression>>> parse_function_parameters();
    std::shared_ptr<ast::Expression> parse_call_expression(std::shared_ptr<ast::Expression> function);
    std::shared_ptr<ast::Expression> parse_string_literal();
    std::shared_ptr<ast::Expression> parse_array_literal();
    std::optional<std::vector<std::shared_ptr<ast::Expression>>> parse_expression_list(tokenize::TokenType end);
    std::shared_ptr<ast::Expression> parse_index_expression(std::shared_ptr<ast::Expression> left);

  private:
    TokenStream stream;
    std::unordered_map<tokenize::TokenType, prefixParseFn> prefixParseFns;
    std::unordered_map<tokenize::TokenType, infixParseFn> infixParseFns;
    std::vector<std::exception_ptr> errors;
};

} // namespace parsing
} // namespace nugdev::compiler