#include <gtest/gtest.h>

#include "01_tokenize/Tokenizer.h"
#include "01_tokenize/factory/KeywordTokenFactory.h"

TEST(KeywordTokenFactory, canHandle) {
    std::wstring data = L"function";
    auto stream = std::wistringstream(data);
    auto tokenizer = nugdev::compiler::tokenize::Tokenizer({std::make_shared<nugdev::compiler::tokenize::KeywordTokenFactory>()});
    auto token = tokenizer.tokenize(stream);
    EXPECT_EQ(token.size(), 1);
    EXPECT_EQ(token[0].to_str(), icu::UnicodeString::fromUTF8("function"));
}

TEST(KeywordTokenFactory, canNotHandle) {
    std::wstring data = L"function123";
    auto stream = std::wistringstream(data);
    auto tokenizer = nugdev::compiler::tokenize::Tokenizer({std::make_shared<nugdev::compiler::tokenize::KeywordTokenFactory>()});
    auto token = tokenizer.tokenize(stream);
    EXPECT_EQ(token.size(), 0);
}

TEST(KeywordTokenFactory, canHandleMultipleWords) {
    std::wstring data = L"function let";
    auto stream = std::wistringstream(data);
    auto tokenizer = nugdev::compiler::tokenize::Tokenizer({std::make_shared<nugdev::compiler::tokenize::KeywordTokenFactory>()});
    auto token = tokenizer.tokenize(stream);
    EXPECT_EQ(token.size(), 2);
    EXPECT_EQ(token[0].to_str(), icu::UnicodeString::fromUTF8("function"));
    EXPECT_EQ(token[1].to_str(), icu::UnicodeString::fromUTF8("let"));
}
