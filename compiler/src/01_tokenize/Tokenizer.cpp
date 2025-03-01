#include "Tokenizer.h"

#include <unicode/unistr.h>

#include "00_app/stream/Stream.hpp"
#include "00_app/stream/StreamWorkbench.hpp"
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
    auto [tokens, _] = stream::workbench(stream, [this](stream::StringStream &workbench) {
        std::vector<Token> tokens;

        while (workbench.current().valid()) {
            while (::iswspace(workbench.current().value())) {
                workbench.advance();
            }

            if (workbench.current().value() == u'#') {
                auto it = find_first_of(workbench, {u'\n'});
                workbench.move_at(it);
                continue;
            }

            auto tokenProcessed = false;
            for (auto &factory : factories) {
                if (!factory->can_handle(workbench)) {
                    continue;
                }

                try {
                    auto [token, next] = factory->create_token(workbench);
                    tokens.push_back(token);
                    workbench.move_at(next);
                    tokenProcessed = true;
                    break;
                } catch (const std::exception &e) {
                    continue;
                }
            }

            if (!tokenProcessed) {
                throw std::runtime_error("infinite loop detected - parser not advancing");
            }
        }

        return tokens;
    });

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
