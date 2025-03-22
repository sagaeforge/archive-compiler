#include "02_parsing/ast/expression/type/TypeLiteralNodeParseStrategy.h"

#include "00_app/stream/StreamWorkbench.hpp"
#include "01_tokenize/Token.h"
#include "02_parsing/TypeMeta.h"
#include "02_parsing/ast/expression/type/TypeLiteralNode.h"

namespace nugdev::compiler::ast::expression {

bool TypeLiteralNodeParseStrategy::can_parse(const parsing::Parser &parser, const tokenize::TokenStream &tokens) {
    return contains(tokens.current(), {tokenize::TokenType::Ident});
}

parsing::ParseStrategyResult TypeLiteralNodeParseStrategy::parse(parsing::Parser &parser, const tokenize::TokenStream &tokens) {
    auto [node, itr] = stream::workbench(tokens, [this](auto &workbench) {
        auto token = workbench.current();

        auto it = typeMap.find(token->get_literal());
        if (it == typeMap.end()) {
            throw std::runtime_error("Invalid primitive type");
        }

        return create_node(*token, it->second);
    });

    return {node, itr};
}

std::shared_ptr<ast::ASTNode> TypeLiteralNodeParseStrategy::create_node(const tokenize::Token &token, const TypeInfo &meta) {
    return std::make_shared<TypeLiteralNode>(token, meta);
}

} // namespace nugdev::compiler::ast::expression
