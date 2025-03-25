#include "ProgramNodeParseStrategy.h"
#include "ProgramNode.h"

#include "02_parsing/ast/statement/StatementParseStrategy.h"

namespace nugdev::compiler::ast::module {

bool ProgramNodeParseStrategy::can_parse(const parsing::Parser &parser, const tokenize::TokenStream &tokens) {
    return true;
}

parsing::ParseStrategyResult ProgramNodeParseStrategy::parse(parsing::Parser &parser, const tokenize::TokenStream &tokens) {
    static statement::StatementParseStrategy strategy{};

    std::vector<std::shared_ptr<Statement>> statements;

    auto workbench = tokens.clone();
    do {
        auto [statement, nextItr] = strategy.parse(parser, workbench);
        workbench.move_at(nextItr);
        statements.push_back(statement->as<Statement>());
    } while (workbench.current().valid() && !contains(workbench.current(), {tokenize::TokenType::EoF}));

    return {std::make_shared<ProgramNode>(statements), tokens.begin() + workbench.current().distance()};
}

}  // namespace nugdev::compiler::ast::module
