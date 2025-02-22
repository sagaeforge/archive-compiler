#pragma once

#include "01_tokenize/Token.h"
#include "02_parsing/ast/AST.h"

namespace nugdev::compiler {

namespace parsing {

class Parser {
  private:
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

  public:
    using TokenStream = tokenize::TokenStream;
    using TokenStreamIterator = tokenize::TokenStreamIterator;

  public:
    Parser() = default;
    ~Parser() = default;

    /*
        root -> statements -> expression
     */

  public:
    std::shared_ptr<ast::Module> parse_root(const TokenStream &tokens);

    std::shared_ptr<ast::Statement> parse_statement();

    std::shared_ptr<ast::Expression> parse_expression();
};

} // namespace parsing
} // namespace nugdev::compiler
