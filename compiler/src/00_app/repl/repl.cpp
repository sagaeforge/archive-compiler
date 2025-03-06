#include "00_app/repl/repl.h"
#include "01_tokenize/Tokenizer.h"
#include "02_parsing/Parser.h"
#include "04_generation/opcode/BytecodeGenerator.h"
#include <iostream>
#include <string>
#include <unicode/unistr.h>

namespace nugdev::compiler::repl {

void Repl::run() {
    std::string line;
    std::cout << "Nugdev Compiler REPL (exit으로 종료)" << std::endl;
    std::cout << "입력 > ";

    while (std::getline(std::cin, line)) {
        if (line.empty()) {
            std::cout << "입력 > ";
            continue;
        }

        if (line == "exit") {
            break;
        }

        // 1. 토큰화
        tokenize::Tokenizer tokenizer;
        auto tokens = tokenizer.tokenize(icu::UnicodeString::fromUTF8(line));

        // 2. 파싱 (AST 생성)
        parsing::Parser parser;
        auto ast = parser.parse(tokens);

        // 3. 바이트코드 생성
        generation::BytecodeGenerator generator;
        generator.generate(ast);

        // 4. 결과 출력
        std::cout << "=== AST ===" << std::endl;
        std::cout << parser.to_string(ast) << std::endl;

        std::cout << "=== 바이트코드 ===" << std::endl;
        std::cout << generator.dumpBytecode() << std::endl;

        std::cout << "입력 > ";
    }

    std::cout << "REPL 종료" << std::endl;
}

} // namespace nugdev::compiler::repl