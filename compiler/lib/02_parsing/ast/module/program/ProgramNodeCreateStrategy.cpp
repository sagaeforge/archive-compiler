#include "ProgramNodeCreateStrategy.h"
#include "ProgramNode.h"

namespace nugdev::compiler::parsing {

bool ProgramNodeCreateStrategy::can_handle(const lib::iterator::Workbench<tokenize::Token>::command_t &command) {
    return true;
}

std::optional<ast::ASTNodePtr> ProgramNodeCreateStrategy::handle(const lib::iterator::Workbench<tokenize::Token>::command_t &command) {
    return {};
}

}  // namespace nugdev::compiler::parsing
