#include <gtest/gtest.h>

#include "01_tokenize/Tokenizer.h"
#include "01_tokenize/factory/OperatorTokenFactory.h"

TEST(OperatorTokenFactory, canHandle) {
    std::wstring data = L"+";
    auto stream = std::wistringstream(data);
    auto tokenizer = nugdev::compiler::tokenize::Tokenizer({std::make_shared<nugdev::compiler::tokenize::OperatorTokenFactory>()});
    auto token = tokenizer.tokenize(stream);
    EXPECT_EQ(token.size(), 1);
    EXPECT_EQ(token[0].to_str(), icu::UnicodeString::fromUTF8("+"));
}

TEST(OperatorTokenFactory, canNotHandle) {
    std::wstring data = L"a";
    auto stream = std::wistringstream(data);
    auto tokenizer = nugdev::compiler::tokenize::Tokenizer({std::make_shared<nugdev::compiler::tokenize::OperatorTokenFactory>()});
    auto token = tokenizer.tokenize(stream);
    EXPECT_EQ(token.size(), 0);
}

TEST(OperatorTokenFactory, canHandleMultipleOperators) {
    std::wstring data = L"++";
    auto stream = std::wistringstream(data);
    auto tokenizer = nugdev::compiler::tokenize::Tokenizer({std::make_shared<nugdev::compiler::tokenize::OperatorTokenFactory>()});
    auto token = tokenizer.tokenize(stream);
    EXPECT_EQ(token.size(), 2);
    EXPECT_EQ(token[0].to_str(), icu::UnicodeString::fromUTF8("+"));
    EXPECT_EQ(token[1].to_str(), icu::UnicodeString::fromUTF8("+"));
}