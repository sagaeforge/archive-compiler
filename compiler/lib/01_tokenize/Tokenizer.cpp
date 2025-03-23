#include "Tokenizer.h"

#include "00_lib/iterator/Workbench.hpp"
#include "01_tokenize/strategies/CommentTokenizeStrategy.h"
#include "01_tokenize/strategies/IdentifierTokenizeStrategy.h"
#include "01_tokenize/strategies/NumberTokenizeStrategy.h"
#include "01_tokenize/strategies/OperatorTokenizeStrategy.h"
#include "01_tokenize/strategies/StringTokenizeStrategy.h"
#include "01_tokenize/strategies/TokenizeStrategy.h"

namespace nugdev::compiler::tokenize {

Tokenizer::Tokenizer()
    : m_strategies{
          std::make_shared<IdentifierTokenizeStrategy>(), std::make_shared<StringTokenizeStrategy>(),  std::make_shared<NumberTokenizeStrategy>(),
          std::make_shared<OperatorTokenizeStrategy>(),   std::make_shared<CommentTokenizeStrategy>(),
      } {}

Tokenizer::Tokenizer(const std::vector<std::shared_ptr<TokenizeStrategy>> &strategies) : m_strategies(strategies) {}

std::vector<Token> Tokenizer::tokenize(const lib::String &resource) {
    auto tokens = lib::iterator::Workbench<lib::Char>::from(resource.to_vector()).stream([this](const lib::iterator::Workbench<lib::Char>::command_t &command) {
        while (command.valid() && ::iswspace(command.value())) {
            command.next();
        }

        for (const auto &strategy : m_strategies) {
            if (command.valid() && strategy->can_handle(command)) {
                if (auto token = strategy->handle(command); token.has_value()) {
                    return token.value();
                }
            }
        }

        return Token(TokenType::Illegal, command.value());
    });

    // Filter tokens
    std::vector<Token> filtered_tokens;
    std::copy_if(tokens.begin(), tokens.end(), std::back_inserter(filtered_tokens), [](const Token &token) { return token.get_type() != TokenType::Comment; });
    return filtered_tokens;
}

} // namespace nugdev::compiler::tokenize
