#include "ProgramNodeParseStrategy.h"
#include "ProgramNode.h"

namespace nugdev::compiler::ast::module {

bool ProgramNodeParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return true; }

parsing::ParseStrategyResult ProgramNodeParseStrategy::parse(const tokenize::TokenStream &tokens) {
    auto statements = std::vector<std::shared_ptr<Statement>>();
    // while (tokens.current() != tokens.end()) {
    //     auto statement = Statement::parse(tokens);
    //     statements.push_back(statement);
    // }
    return parsing::ParseStrategyResult(std::make_shared<ProgramNode>(statements), tokens.current());
}

} // namespace nugdev::compiler::ast::module
