//
// Created by lambda on 11/1/25.
//

#include "02_parsing/parser.h"

#include <gtest/gtest.h>

#include "01_tokenize/tokenizer.h"
#include "02_parsing/ast/util/ast_json_exporter.h"
#include "02_parsing/ast/module/program.h"

TEST(milestone_01_fibonacci, shows_ast) {
    auto file = std::ifstream("../test/milestone/1/fibonacci.txt");
    auto tokenizer = Tokenizer("fibonacci.txt", file);
    auto tokens = tokenizer.tokenize();
    auto parser = Parser(tokens);
    auto node = parser.parse()->as<Program>();

    auto exporter = ASTJsonExporter();
    exporter.visit(node);

    std::cout << exporter.toJson() << std::endl;
}

TEST(milestone_01_fibonacci, parsing) {
    auto file = std::ifstream("../test/milestone/1/fibonacci.txt");
    auto tokenizer = Tokenizer("fibonacci.txt", file);
    auto tokens = tokenizer.tokenize();
    auto parser = Parser(tokens);
    auto node = parser.parse()->as<Program>();

    auto exporter = ASTJsonExporter();
    exporter.visit(node);

    std::ifstream expectedFile("../test/milestone/1/02. parsing/expected.json");
    string_t fileContent;
    string_t line;
    while (getline(expectedFile, line)) {
        fileContent += line + '\n';
    }
    nlohmann::json expectedJson = nlohmann::json::parse(fileContent);

    ASSERT_EQ(expectedJson, exporter.toJson());
}
