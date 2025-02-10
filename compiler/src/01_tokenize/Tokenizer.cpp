#include "Tokenizer.h"

#include <unicode/unistr.h>

#include "Token.h"
#include "factory/IdentifierTokenFactory.h"

namespace nugdev::compiler::tokenize {

Tokenizer::Tokenizer() {
    factories.push_back(std::make_shared<IdentifierTokenFactory>());
    // factories.push_back(std::make_shared<OperatorTokenFactory>());
}

std::vector<Token> Tokenizer::tokenize(std::wistream &stream) {
    if (!stream || stream.eof()) {
        return {};
    }

    std::vector<Token> tokens;
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
                continue;
            }
        }

        // 원래는 이상한 케이스라, 예외가 나야하지만, 현재는 개발 상황이고, 이상한 케이스들이 많이 나올 예정이라.
        stream.ignore();
    }

    return tokens;
}

} // namespace nugdev::compiler::tokenize
