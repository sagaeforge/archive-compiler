#include "../token_test_case.h"

#include "01_tokenize/Tokenizer.h"
#include <gtest/gtest.h>
#include <unicode/unistr.h>

TEST_F(TokenTestCase, InitToken) {
    EXPECT_TRUE(load_sample("init_token"));

    auto codeLines = get_code_lines();
    auto tokenLines = get_token_lines();

    for (size_t i = 0; i < codeLines.size(); i++) {
        auto codeLine = codeLines[i];
        // 만약에 codeLine이 공백일 경우 스킵
        if (codeLine.trim().isEmpty()) {
            continue;
        }

        icu::UnicodeString tokenLine = tokenLines[i];

        nugdev::compiler::tokenize::Tokenizer tokenizer;
        auto tokens = tokenizer.tokenize(codeLine);

        for (const auto &token : tokens) {
            const auto &tokenStr = std::to_string(token.get_type());
            const auto utf8TokenStr = icu::UnicodeString::fromUTF8(tokenStr);
            if (utf8TokenStr.compare(tokenLine) != 0) {
                std::string stdStr1;
                tokenLine.toUTF8String(stdStr1);
                std::string stdStr2;
                utf8TokenStr.toUTF8String(stdStr2);
                throw std::runtime_error("token line mismatch: " + stdStr1 + " != " + stdStr2);
            }
            EXPECT_TRUE(utf8TokenStr.compare(tokenLine) == 0);
        }
    }
}
