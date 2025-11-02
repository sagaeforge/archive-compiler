//
// Created by lambda on 10/31/25.
//

#include "01_tokenize/tokenizer.h"

#include <gtest/gtest.h>

#include "01_tokenize/token_converter.h"

TEST(milestone_01_fibonacci, shows_token) {
    auto file = std::ifstream("../test/milestone/1/fibonacci.txt");
    auto tokenizer = Tokenizer("fibonacci.txt", file);
    auto tokens = tokenizer.tokenize();

    for (const auto &token: tokens) {
        std::cout << token << std::endl;
    }
}

TEST(milestone_01_fibonacci, tokenize) {
    auto file = std::ifstream("../test/milestone/1/fibonacci.txt");
    auto tokenizer = Tokenizer("fibonacci.txt", file);
    auto tokens = tokenizer.tokenize();

    std::ifstream expectedFile("../test/milestone/1/01. tokenize/expected.json");
    string_t fileContent;
    string_t line;
    while (getline(expectedFile, line)) {
        fileContent += line + '\n';
    }
    nlohmann::json expectedJson = nlohmann::json::parse(fileContent);

    ASSERT_EQ(expectedJson.size(), tokens.size());

    for (auto i = 0; i < tokens.size(); i++) {
        nlohmann::json expectedTokenJson = expectedJson[i];
        auto expectedToken = TokenConverter::importJson(expectedTokenJson);
        auto token = tokens[i];

        // weak 비교
        ASSERT_EQ(expectedToken, token);

        // strong 비교
        ASSERT_EQ(expectedToken.fileName(), token.fileName());
        ASSERT_EQ(expectedToken.line(), token.line());
        ASSERT_EQ(expectedToken.column(), token.column());
    }

    file.close();
}
