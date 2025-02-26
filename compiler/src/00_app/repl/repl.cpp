#include "00_app/repl/repl.h"

#include <iostream>
#include <rapidjson/prettywriter.h>

#include "01_tokenize/Tokenizer.h"
#include "02_parsing/Parser.h"

namespace nugdev::compiler::repl {

void Repl::run() {
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) {
            break;
        }

        if (line == "exit") {
            break;
        }

        tokenize::Tokenizer tokenizer;
        auto tokens = tokenizer.tokenize(icu::UnicodeString::fromUTF8(line));

        parsing::Parser parser;
        auto ast = parser.parse(tokens);

        std::cout << parser.to_string(ast) << std::endl;
    }
}

} // namespace nugdev::compiler::repl