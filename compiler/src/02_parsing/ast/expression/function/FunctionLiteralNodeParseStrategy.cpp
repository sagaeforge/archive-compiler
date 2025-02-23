#include "FunctionLiteralNodeParseStrategy.h"

#include "02_parsing/ast/expression/identifier/IdentifierLiteralNodeParseStrategy.h"
#include "FunctionLiteralNode.h"

namespace nugdev::compiler::ast::expression {

bool FunctionLiteralNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return tokens.current()->get_type() == tokenize::TokenType::Function; }

parsing::ParseStrategyResult FunctionLiteralNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    auto stream = tokens.clone();
    auto itr = stream.current().next();

    if (itr->get_type() != tokenize::TokenType::LParen) {
        throw std::runtime_error("Expected '('");
    }

    if (itr.next()->get_type() == tokenize::TokenType::RParen) {
        return parsing::ParseStrategyResult{std::make_shared<FunctionLiteralNode>(*itr, std::vector<std::shared_ptr<Expression>>(), nullptr),
                                            stream.current() + itr.distance()};
    }

    stream.move(itr.next());
    itr = stream.current();

    auto identifiers = std::vector<std::shared_ptr<Expression>>();
    auto [firstArg, moveItr] = IdentifierLiteralNodeParseStrategy().parse(stream);
    if (firstArg != nullptr) {
        identifiers.push_back(firstArg->as<ast::Expression>());
    }

    while (itr.next()->get_type() == tokenize::TokenType::Comma) {
        stream.move(itr.next().next());
        auto [arg, moveItr] = IdentifierLiteralNodeParseStrategy().parse(stream);
        if (arg != nullptr) {
            identifiers.push_back(arg->as<ast::Expression>());
        }
    }

    if (itr.next()->get_type() != tokenize::TokenType::RParen) {
        throw std::runtime_error("Expected ')'");
    }

    return parsing::ParseStrategyResult{std::make_shared<FunctionLiteralNode>(*itr, identifiers, nullptr), stream.current() + itr.distance()};
}

} // namespace nugdev::compiler::ast::expression
