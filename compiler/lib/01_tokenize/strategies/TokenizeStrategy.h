#pragma once

#include "00_lib/iterator/Workbench.hpp"
#include "00_lib/lib/Strategy.hpp"
#include "01_tokenize/Token.h"

namespace nugdev::compiler::tokenize {

class TokenizeStrategy : public lib::Strategy<Token(const lib::iterator::Workbench<lib::Char>::command_t &)> {};

} // namespace nugdev::compiler::tokenize
