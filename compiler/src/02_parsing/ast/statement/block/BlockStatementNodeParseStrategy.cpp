#include "BlockStatementNodeParseStrategy.h"

#include "00_app/stream/StreamWorkbench.hpp"
#include "02_parsing/ast/statement/StatementParseStrategy.h"
#include "02_parsing/ast/statement/block/BlockStatementNode.h"

namespace nugdev::compiler::ast::statement {

bool BlockStatementNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return true; }

parsing::ParseStrategyResult BlockStatementNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    static StatementParseStrategy strategy{};

    auto [node, itr] = stream::workbench(tokens, [this](tokenize::TokenStream &workbench) {
        std::vector<std::shared_ptr<Statement>> statements;

        do {
            workbench.next();
            if (contains(workbench.current(), {tokenize::TokenType::RBrace})) {
                break;
            }

            auto [statement, itr] = strategy.parse(workbench);
            workbench.move_at(itr);
            statements.push_back(statement->as<Statement>());
        } while (workbench.current().valid() && !contains(workbench.current(), {tokenize::TokenType::RBrace}));
        workbench.next();

        return std::make_shared<BlockStatementNode>(statements);
    });

    return {node, itr};
}

} // namespace nugdev::compiler::ast::statement
