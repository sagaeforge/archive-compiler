#include "Tokenizer.h"

#include <unicode/unistr.h>

#include "Token.h"
#include "TokenFactory.h"

#include "identifier/IdentifierTokenFactory.h"

namespace nugdev::compiler::tokenize {
Tokenizer::Tokenizer() { factories.push_back(std::make_shared<IdentifierTokenFactory>()); }

std::vector<std::shared_ptr<Token>> Tokenizer::tokenize(std::wistream &stream) {
    if (!stream || stream.eof()) {
        return {};
    }

    std::vector<std::shared_ptr<Token>> tokens;
    while (stream && !stream.eof()) {
        auto ch = stream.peek();
        if (::iswspace(ch)) {
            continue;
        }

        // comment 처리
        if (ch == L'#') {
            stream.ignore(std::numeric_limits<std::streamsize>::max(), L'\n');
            continue;
        }

        for (auto &factory : factories) {
            if (factory->canHandle(ch)) {
                auto token = factory->createToken(stream);
                tokens.push_back(token);
            }
        }
    }

    return tokens;
}

} // namespace nugdev::compiler::tokenize
