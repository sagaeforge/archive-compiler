#include "Tokenizer.h"

#include <ranges>
#include <unicode/unistr.h>

#include "00_app/stream/Stream.hpp"
#include "factory/IdentifierTokenFactory.h"
#include "factory/KeywordTokenFactory.h"
#include "factory/NumberTokenFactory.h"
#include "factory/OperatorTokenFactory.h"
#include "factory/StringTokenFactory.h"

namespace nugdev::compiler::tokenize {

Tokenizer::Tokenizer() {
    factories.push_back(std::make_shared<OperatorTokenFactory>());
    factories.push_back(std::make_shared<KeywordTokenFactory>());
    factories.push_back(std::make_shared<IdentifierTokenFactory>());
    factories.push_back(std::make_shared<StringTokenFactory>());
    factories.push_back(std::make_shared<NumberTokenFactory>());
}

Tokenizer::Tokenizer(std::vector<std::shared_ptr<TokenFactory>> factories) { this->factories = factories; }

std::vector<Token> Tokenizer::tokenize(const icu::UnicodeString &str) {
    stream::Stream stream(str);

    std::vector<Token> tokens;
    do {
        const auto &checkpoint = stream.checkpoint();
        if (!stream.is_vaild(checkpoint)) {
            break;
        }

        // 화이트 스페이스도 씹어야 함.
        if (::iswspace(*checkpoint)) {
            stream.advance();
            continue;
        }

        // 주석의 경우 씹어주는 과정이 필요함.
        if (*checkpoint == u'#') {
            auto it = stream.find_first_of({u'\n'});
            stream.commit(it);
            continue;
        }

        auto matchingFactories = factories | std::views::filter([checkpoint](const auto &factory) { return factory->canHandle(checkpoint); });
        if (std::ranges::empty(matchingFactories)) {
            throw std::runtime_error("token factory is not defined");
        }

        auto tokenProcessed = false;
        for (auto &factory : matchingFactories) {
            try {
                auto [token, next] = factory->createToken(checkpoint);
                tokens.push_back(token);
                stream.commit(next);
                tokenProcessed = true;
                break;
            } catch (const std::exception &e) {
                continue;
            }
        }

        if (!tokenProcessed) {
            throw std::runtime_error("infinite loop detected - parser not advancing");
        }
    } while (true);

    return tokens;
}

} // namespace nugdev::compiler::tokenize
