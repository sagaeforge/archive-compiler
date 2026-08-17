#pragma once

#include "01_tokenize/Token.h"
#include "02_parsing/TypeMeta.h"
#include "02_parsing/ast/AST.h"

namespace nugdev::compiler::parsing {

// 조금 더 명확한 객체 구조를 가질 건데, 파싱에 대한 책임을 전부 가질거임.
class Parser {
  public:
    enum class Precedence {
        Lowest = 1,
        In,          // in
        Equals,      // ==
        LessGreater, // > or <
        Sum,         // +
        Product,     // *
        Postfix,     // x++, x--
        Prefix,      // -X, !X ++x, --x
        Call,        // myFunction(X)
        Index,       // array[index]
        Unknown,
    };

  public:
    Parser() = default;
    ~Parser() = default;

  public:
    std::shared_ptr<ast::Module> parse(const tokenize::TokenStream &tokens);

  public:
    struct Mangling {
        lib::String name;
        std::vector<ast::TypeInfo> paramTypes;

        // {version}_{name}_{paramTypes}
        lib::String mangle() const;
    };
    void add_function_symbol_fetch(const Mangling &mangling, const std::function<void(const ast::ASTNodePtr &, const ast::ASTNodePtr &)> &fetcher,
                                   const ast::ASTNodePtr &self);

  public:
    Precedence get_precedence(tokenize::TokenType type) const;
};
using Precedence = Parser::Precedence;

} // namespace nugdev::compiler::parsing
