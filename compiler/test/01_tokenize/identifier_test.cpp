#include <gtest/gtest.h>
#include <unicode/unistr.h>

#include "01_tokenize/Tokenizer.h"

TEST(IdentifierTokenFactory, canHandle) {
    std::wstring data = L"test";
    auto stream = std::wistringstream(data);
    auto tokenizer = nugdev::compiler::tokenize::Tokenizer();
    auto token = tokenizer.tokenize(stream);
    EXPECT_EQ(token.size(), 1);
    EXPECT_EQ(token[0].to_str(), icu::UnicodeString::fromUTF8("test"));
}

TEST(IdentifierTokenFactory, canNotHandle) {
    std::wstring data = L"123";
    auto stream = std::wistringstream(data);
    auto tokenizer = nugdev::compiler::tokenize::Tokenizer();
    auto token = tokenizer.tokenize(stream);
    EXPECT_EQ(token.size(), 0);
}

TEST(IdentifierTokenFactory, canHandleMultipleWords) {
    std::wstring data = L"test test";
    auto stream = std::wistringstream(data);
    auto tokenizer = nugdev::compiler::tokenize::Tokenizer();
    auto token = tokenizer.tokenize(stream);
    EXPECT_EQ(token.size(), 2);
    EXPECT_EQ(token[0].to_str(), icu::UnicodeString::fromUTF8("test"));
    EXPECT_EQ(token[1].to_str(), icu::UnicodeString::fromUTF8("test"));
}
