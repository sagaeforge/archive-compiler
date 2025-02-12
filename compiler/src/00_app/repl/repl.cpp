#include "00_app/repl/repl.h"

#include "01_tokenize/Tokenizer.h"
#include <iostream>

namespace nugdev::compiler::repl {

void Repl::run() {
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) {
            break;
        }

        tokenize::Tokenizer tokenizer;
        auto tokens = tokenizer.tokenize(icu::UnicodeString::fromUTF8(line));
        for (auto token : tokens) {
            std::string str;
            token.to_str().toUTF8String(str);
            std::cout << str << std::endl;
        }
    }
}

} // namespace nugdev::compiler::repl