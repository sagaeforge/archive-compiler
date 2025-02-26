#pragma once

#include "00_app/json/Json.hpp"
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

  public:
    std::shared_ptr<ast::Module> parse(const TokenStream &tokens);
    json::JsonDocument to_json(const std::shared_ptr<ast::Module> &module);
    std::string to_string(const std::shared_ptr<ast::Module> &module);
};

} // namespace parsing
} // namespace nugdev::compiler
