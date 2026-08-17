#include "StringLiteralNodeParseStrategy.h"

#include "00_app/stream/StreamWorkbench.hpp"
#include "02_parsing/ast/expression/string/StringLiteralNode.h"

namespace nugdev::compiler::ast::expression {

bool StringLiteralNodeParseStrategy::can_parse(const parsing::Parser &parser, const tokenize::TokenStream &tokens) {
    return contains(tokens.current(), {tokenize::TokenType::String});
}

parsing::ParseStrategyResult StringLiteralNodeParseStrategy::parse(parsing::Parser &parser, const tokenize::TokenStream &tokens) {
    auto [node, itr] = stream::workbench(tokens, [this, &parser](tokenize::TokenStream &workbench) {
        auto stringToken = workbench.current();
        workbench.move_next();
        return std::make_shared<StringLiteralNode>(stringToken.value(), stringToken->get_literal());
    });
    return {node, itr};
}

} // namespace nugdev::compiler::ast::expression