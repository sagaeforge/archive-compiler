#include "Tokenizer.h"

#include "00_lib/iterator/Workbench.hpp"
#include "01_tokenize/strategies/IdentifierTokenizeStrategy.h"
#include "01_tokenize/strategies/NumberTokenizeStrategy.h"
#include "01_tokenize/strategies/OperatorTokenizeStrategy.h"
#include "01_tokenize/strategies/StringTokenizeStrategy.h"
#include "01_tokenize/strategies/TokenizeStrategy.h"

namespace nugdev::compiler::tokenize {

Tokenizer::Tokenizer()
    : m_strategies{
          std::make_shared<IdentifierTokenizeStrategy>(),
          std::make_shared<StringTokenizeStrategy>(),
          std::make_shared<NumberTokenizeStrategy>(),
          std::make_shared<OperatorTokenizeStrategy>(),
      } {}

Tokenizer::Tokenizer(const std::vector<std::shared_ptr<TokenizeStrategy>> &strategies) : m_strategies(strategies) {}

std::vector<Token> Tokenizer::tokenize(const lib::String &resource) {
    return lib::iterator::Workbench<lib::Char>::from(resource.to_vector()).stream([this](const lib::iterator::Workbench<lib::Char>::command_t &command) {
        for (; command.valid() && ::iswspace(command.value()); command.next())
            ;

        for (const auto &strategy : m_strategies) {
            if (strategy->can_handle(command)) {
                if (auto token = strategy->handle(command); token.has_value()) {
                    return token.value();
                }
            }
        }

        return Token(TokenType::Illegal, command.value());
    });
}

} // namespace nugdev::compiler::tokenize
