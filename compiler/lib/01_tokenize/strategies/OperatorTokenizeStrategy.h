#pragma once

#include "01_tokenize/strategies/TokenizeStrategy.h"

namespace nugdev::compiler::tokenize {

class OperatorTokenizeStrategy : public TokenizeStrategy {
public:
    OperatorTokenizeStrategy();

public:
    bool can_handle(const lib::iterator::Workbench<lib::Char>::command_t &command) override;
    std::optional<Token> handle(const lib::iterator::Workbench<lib::Char>::command_t &command) override;

private:
    std::unordered_map<wchar_t, TokenType> m_operatorMap;
};

}  // namespace nugdev::compiler::tokenize