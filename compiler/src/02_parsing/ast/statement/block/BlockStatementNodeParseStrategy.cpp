#include "BlockStatementNodeParseStrategy.h"

#include "01_tokenize/Token.h"
#include "02_parsing/ast/statement/StatementParseStrategy.h"
#include "02_parsing/ast/statement/block/BlockStatementNode.h"

namespace nugdev::compiler::ast::statement {

bool BlockStatementNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return true; }

parsing::ParseStrategyResult BlockStatementNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    static StatementParseStrategy strategy{};

    std::vector<std::shared_ptr<Statement>> statements;

    auto workbench = tokens.clone(); // current : '{'
    do {
        auto [statement, itr] = strategy.parse(workbench);
        workbench.move_at(itr);
        statements.push_back(statement->as<Statement>());
    } while (workbench.current().valid() && contains(workbench.current(), {tokenize::TokenType::RBrace, tokenize::TokenType::EoF}));

    return {std::make_shared<BlockStatementNode>(statements), tokens.begin() + workbench.current().distance()};
}

} // namespace nugdev::compiler::ast::statement
