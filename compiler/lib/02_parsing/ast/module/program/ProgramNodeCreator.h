#pragma once

#include "02_parsing/ast/ASTNodeCreateStrategy.h"

namespace nugdev::compiler::ast::module {

class ProgramNodeCreator : public ASTNodeCreateStrategy {
public:
    bool can_handle(const lib::iterator::Workbench<tokenize::Token>::command_t &command) override;
    std::optional<ASTNodePtr> handle(const lib::iterator::Workbench<tokenize::Token>::command_t &command) override;
};

}  // namespace nugdev::compiler::ast::module