//
// Created by lambda on 11/23/25.
//

#include <fstream>
#include "gtest/gtest.h"

#include "01_tokenize/tokenizer.h"
#include "02_parsing/parser.h"
#include "02_parsing/ast/module/program.h"
#include "04_generate/compiler.h"

TEST(milestone_01_fibonacci, shows_code) {
    auto file = std::ifstream("../test/milestone/1/fibonacci.txt");
    auto tokenizer = Tokenizer("fibonacci.txt", file);
    auto tokens = tokenizer.tokenize();
    auto parser = Parser(tokens);
    auto node = parser.parse()->as<Program>();
    auto compiler = Compiler();
    compiler.visit(node);

    std::cout << compiler.generate() << std::endl;
}
