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
    auto stream = stream::make_stream(str);

    std::vector<Token> tokens;
    do {
        const auto &current = stream.current();
        if (!stream.is_valid(current)) {
            break;
        }

        // 화이트 스페이스도 씹어야 함.
        if (::iswspace(*current)) {
            stream.advance();
            continue;
        }

        // 주석의 경우 씹어주는 과정이 필요함.
        if (*current == u'#') {
            auto it = find_first_of(stream, {u'\n'});
            stream.move(it);
            continue;
        }

        auto matchingFactories = factories | std::views::filter([current](const auto &factory) { return factory->can_handle(current); });
        if (std::ranges::empty(matchingFactories)) {
            std::string stdStr;
            str.toUTF8String(stdStr);
            throw std::runtime_error("token factory is not defined: " + stdStr);
        }

        auto tokenProcessed = false;
        for (auto &factory : matchingFactories) {
            try {
                auto [token, next] = factory->create_token(current);
                tokens.push_back(token);
                stream.move(next);
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

stream::StringStreamIterator Tokenizer::find_first_of(const stream::StringStream &stream, const std::vector<char16_t> &chars) {
    for (auto it = stream.current(); it != stream.end(); ++it) {
        if (std::ranges::find(chars, *it) != chars.end()) {
            return it;
        }
    }
    return stream.end();
}
} // namespace nugdev::compiler::tokenize
