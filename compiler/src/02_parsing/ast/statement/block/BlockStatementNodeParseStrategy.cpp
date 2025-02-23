#include "BlockStatementNodeParseStrategy.h"

#include "02_parsing/ast/statement/StatementParseStrategy.h"
#include "02_parsing/ast/statement/block/BlockStatementNode.h"

namespace nugdev::compiler::ast::statement {

bool BlockStatementNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return true; }

parsing::ParseStrategyResult BlockStatementNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    auto workbench = tokens.clone();
    auto itr = workbench.current();
    itr = itr.next();

    static auto strategy = std::make_shared<StatementParseStrategy>();

    std::vector<std::shared_ptr<Statement>> statements;
    while (itr.valid() && itr.value().get_type() != tokenize::TokenType::RBrace && itr.value().get_type() != tokenize::TokenType::EoF) {
        auto statement = strategy->parse(workbench.move(itr.distance()));
        if (statement.node != nullptr) {
            statements.push_back(statement.node->as<Statement>());
        }
        itr = itr.next();
    }

    return parsing::ParseStrategyResult(std::make_shared<BlockStatementNode>(tokens.current().value(), statements), tokens.current() + itr.distance());
}

} // namespace nugdev::compiler::ast::statement
