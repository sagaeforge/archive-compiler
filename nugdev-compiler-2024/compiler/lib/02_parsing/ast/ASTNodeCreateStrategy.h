#pragma once

#include "00_lib/iterator/Workbench.hpp"
#include "00_lib/lib/Strategy.hpp"
#include "01_tokenize/token/Token.h"
#include "02_parsing/ast/ASTNode.h"

namespace nugdev::compiler::ast {

class ParseCommand {};

class ASTNodeCreateStrategy : public lib::Strategy<ASTNodePtr(const lib::iterator::Workbench<tokenize::Token>::command_t &)> {};

}  // namespace nugdev::compiler::ast