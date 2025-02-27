#include "StatementParseStrategy.h"

#include "02_parsing/ast/statement/break/BreakNodeParseStrategy.h"
#include "02_parsing/ast/statement/continue/ContinueNodeParseStrategy.h"
#include "02_parsing/ast/statement/expression/ExpressionStatementNodeParseStrategy.h"
#include "02_parsing/ast/statement/for/ForNodeParseStrategy.h"
#include "02_parsing/ast/statement/let/LetNodeParseStrategy.h"
#include "02_parsing/ast/statement/return/ReturnNodeParseStrategy.h"

namespace nugdev::compiler::ast::statement {

bool StatementParseStrategy::can_parse(const tokenize::TokenStream &tokens) { return true; }

parsing::ParseStrategyResult StatementParseStrategy::parse(const tokenize::TokenStream &tokens) {
    std::vector<std::shared_ptr<parsing::ParseStrategy>> strategies{
        std::make_shared<BreakNodeParseStrategy>(),
        std::make_shared<ContinueNodeParseStrategy>(),
        std::make_shared<ReturnNodeParseStrategy>(),
        std::make_shared<LetNodeParseStrategy>(),
        std::make_shared<ExpressionStatementNodeParseStrategy>(),
        std::make_shared<ForNodeParseStrategy>(),
    };

    for (auto &strategy : strategies) {
        if (strategy->can_parse(tokens) == false) {
            continue;
        }

        return strategy->parse(tokens);
    }

    return parsing::ParseStrategyResult(nullptr, tokens.current());
}

} // namespace nugdev::compiler::ast::statement
