#include "Parser.h"

#include "02_parsing/ast/module/program/ProgramNodeParseStrategy.h"

namespace nugdev::compiler::parsing {

std::shared_ptr<ast::Module> Parser::parse(const tokenize::TokenStream &tokens) {
    static ast::module::ProgramNodeParseStrategy strategy{};

    auto [node, _] = strategy.parse(*this, tokens);
    return node->as<ast::Module>();
}

Parser::Precedence Parser::get_precedence(tokenize::TokenType type) const {
    switch (type) {
    case tokenize::TokenType::In:
        return Precedence::In;
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
    case tokenize::TokenType::Inc:
    case tokenize::TokenType::Dec:
        return Precedence::Postfix;
    case tokenize::TokenType::LParen:
        return Precedence::Call;
    case tokenize::TokenType::LBracket:
        return Precedence::Index;
    default:
        return Precedence::Lowest;
    }
}
} // namespace nugdev::compiler::parsing
