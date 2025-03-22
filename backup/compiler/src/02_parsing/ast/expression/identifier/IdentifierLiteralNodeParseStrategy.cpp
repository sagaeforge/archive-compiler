#include "IdentifierLiteralNodeParseStrategy.h"

#include "00_app/stream/StreamWorkbench.hpp"
#include "02_parsing/ast/expression/identifier/IdentifierLiteralNode.h"

namespace nugdev::compiler::ast::expression {

bool IdentifierLiteralNodeParseStrategy::can_parse(const parsing::Parser &parser, const tokenize::TokenStream &tokens) {
    return tokens.current().valid() && contains(tokens.current(), {tokenize::TokenType::Ident});
}

parsing::ParseStrategyResult IdentifierLiteralNodeParseStrategy::parse(parsing::Parser &parser, const tokenize::TokenStream &tokens) {
    auto [node, itr] = stream::workbench(tokens, [this, &parser](tokenize::TokenStream &workbench) {
        auto identifierToken = workbench.current();
        workbench.move_next();
        return std::make_shared<IdentifierLiteralNode>(identifierToken.value(), identifierToken->get_literal());
    });
    return {node, itr};
}

} // namespace nugdev::compiler::ast::expression
