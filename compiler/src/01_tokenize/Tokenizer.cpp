#include "Tokenizer.h"

#include <cstdio>
#include <unicode/unistr.h>

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

std::vector<Token> Tokenizer::tokenize(std::wistream &stream) {
    if (!stream || stream.eof()) {
        return {};
    }

    std::vector<Token> tokens;
    while (stream && !stream.eof() && stream.peek() != EOF) {
        auto ch = stream.peek();
        if (::iswspace(ch)) {
            stream.ignore();
            continue;
        }

        // comment 처리
        if (ch == L'#') {
            stream.ignore(std::numeric_limits<std::streamsize>::max(), L'\n');
            continue;
        }

        auto pos = stream.tellg();
        bool continueSignal = false;
        for (auto &factory : factories) {
            if (factory->canHandle(ch)) {
                try {
                    auto token = factory->createToken(stream);
                    tokens.push_back(token);
                    stream.seekg(-1, std::ios::cur);
                    continueSignal = true;
                    break;
                } catch (const std::exception &e) {
                    // 예외가 나면, 현재 포지션으로 돌아가기.
                    stream.seekg(pos);
                    continue;
                }
            }
        }

        if (continueSignal) {
            continue;
        }

        // 원래는 이상한 케이스라, 예외가 나야하지만, 현재는 개발 상황이고, 이상한 케이스들이 많이 나올 예정이라.
        stream.ignore();
    }

    return tokens;
}

} // namespace nugdev::compiler::tokenize
