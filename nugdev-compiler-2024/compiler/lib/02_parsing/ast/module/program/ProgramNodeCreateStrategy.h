#pragma once

#include "02_parsing/ast/ASTNodeCreateStrategy.h"

namespace nugdev::compiler::parsing {

class ProgramNodeCreateStrategy : public ast::ASTNodeCreateStrategy {
public:
    bool can_handle(const lib::iterator::Workbench<tokenize::Token>::command_t &command) override;
    std::optional<ast::ASTNodePtr> handle(const lib::iterator::Workbench<tokenize::Token>::command_t &command) override;
};

}  // namespace nugdev::compiler::ast::module