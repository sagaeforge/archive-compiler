#include "ProgramNodeParseStrategy.h"
#include "ProgramNode.h"

#include "02_parsing/ast/statement/StatementParseStrategy.h"

namespace nugdev::compiler::ast::module {

bool ProgramNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return true; }

parsing::ParseStrategyResult ProgramNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    static statement::StatementParseStrategy strategy{};

    std::vector<std::shared_ptr<Statement>> statements;

    auto workbench = tokens.clone();
    auto itr = workbench.current();
    do {
        itr = workbench.current();

        auto [statement, nextItr] = strategy.parse(workbench.move(itr.distance()));
        workbench.move_at(nextItr);
        statements.push_back(statement->as<Statement>());
    } while (itr.valid() && itr.value().get_type() != tokenize::TokenType::EoF);

    return {std::make_shared<ProgramNode>(statements), tokens.begin() + workbench.current().distance()};
}

} // namespace nugdev::compiler::ast::module
