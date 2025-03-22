#include "StatementParseStrategy.h"

#include "02_parsing/ast/statement/break/BreakStatementNodeParseStrategy.h"
#include "02_parsing/ast/statement/continue/ContinueStatementNodeParseStrategy.h"
#include "02_parsing/ast/statement/expression/ExpressionStatementNodeParseStrategy.h"
#include "02_parsing/ast/statement/for/ForStatementNodeParseStrategy.h"
#include "02_parsing/ast/statement/let/LetStatementNodeParseStrategy.h"
#include "02_parsing/ast/statement/return/ReturnStatementNodeParseStrategy.h"

namespace nugdev::compiler::ast::statement {

bool StatementParseStrategy::can_parse(const parsing::Parser &parser, const tokenize::TokenStream &tokens) { return true; }

parsing::ParseStrategyResult StatementParseStrategy::parse(parsing::Parser &parser, const tokenize::TokenStream &tokens) {
    std::vector<std::shared_ptr<parsing::ParseStrategy>> strategies{
        std::make_shared<BreakStatementNodeParseStrategy>(),  std::make_shared<ContinueStatementNodeParseStrategy>(),
        std::make_shared<ReturnStatementNodeParseStrategy>(), std::make_shared<LetStatementNodeParseStrategy>(),
        std::make_shared<ForStatementNodeParseStrategy>(),    std::make_shared<ExpressionStatementNodeParseStrategy>(),
    };

    for (auto &strategy : strategies) {
        if (strategy->can_parse(parser, tokens) == false) {
            continue;
        }

        return strategy->parse(parser, tokens);
    }

    return parsing::ParseStrategyResult(nullptr, tokens.current());
}

} // namespace nugdev::compiler::ast::statement
