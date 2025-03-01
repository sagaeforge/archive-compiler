#include "IdentifierLiteralNodeParseStrategy.h"

#include "00_app/stream/StreamWorkbench.hpp"
#include "02_parsing/ast/expression/identifier/IdentifierLiteralNode.h"

namespace nugdev::compiler::ast::expression {

bool IdentifierLiteralNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) {
    return tokens.current().valid() && contains(tokens.current(), {tokenize::TokenType::Ident});
}

parsing::ParseStrategyResult IdentifierLiteralNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    auto [node, itr] = stream::workbench(tokens, [this, &tokens](tokenize::TokenStream &workbench) {
        auto itr = workbench.current();
        workbench.next();
        return std::make_shared<IdentifierLiteralNode>(tokens.current().value(), itr->get_literal());
    });
    return {node, itr};
}

} // namespace nugdev::compiler::ast::expression
