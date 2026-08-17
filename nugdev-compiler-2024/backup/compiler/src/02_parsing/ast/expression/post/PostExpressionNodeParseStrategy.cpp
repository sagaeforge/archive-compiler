#include "PostExpressionNodeParseStrategy.h"

#include "00_app/stream/StreamWorkbench.hpp"
#include "02_parsing/ast/expression/post/PostExpressionNode.h"

namespace nugdev::compiler::ast::expression {

bool PostExpressionNodeParseStrategy::can_parse(const parsing::Parser &parser, const tokenize::TokenStream &tokens) {
    return contains(tokens.current(), {tokenize::TokenType::Inc, tokenize::TokenType::Dec});
}

parsing::ParseStrategyResult PostExpressionNodeParseStrategy::parse(parsing::Parser &parser, const tokenize::TokenStream &tokens) {
    throw std::runtime_error("Not implemented");
}

parsing::ParseStrategyResult PostExpressionNodeParseStrategy::parse(parsing::Parser &parser, const tokenize::TokenStream &tokens,
                                                                    std::shared_ptr<Expression> left) {
    // current: '++' | '--'
    auto [node, itr] = stream::workbench(tokens, [this, &parser, left](tokenize::TokenStream &workbench) {
        auto postToken = workbench.current();
        workbench.move_next();
        return std::make_shared<PostExpressionNode>(postToken.value(), left, postToken->get_literal());
    });
    return {node, itr};
}

} // namespace nugdev::compiler::ast::expression
