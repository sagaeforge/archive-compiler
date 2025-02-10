#include <gtest/gtest.h>
#include <unicode/unistr.h>

#include "01_tokenize/Tokenizer.h"
#include "01_tokenize/factory/NumberTokenFactory.h"

TEST(NumberTokenFactory, canHandle) {
    std::wstring data = L"123";
    auto stream = std::wistringstream(data);
    auto tokenizer = nugdev::compiler::tokenize::Tokenizer({std::make_shared<nugdev::compiler::tokenize::NumberTokenFactory>()});
    auto token = tokenizer.tokenize(stream);
    EXPECT_EQ(token.size(), 1);
    EXPECT_EQ(token[0].to_str(), icu::UnicodeString::fromUTF8("123"));
}

TEST(NumberTokenFactory, canNotHandle) {
    std::wstring data = L"test";
    auto stream = std::wistringstream(data);
    auto tokenizer = nugdev::compiler::tokenize::Tokenizer({std::make_shared<nugdev::compiler::tokenize::NumberTokenFactory>()});
    auto token = tokenizer.tokenize(stream);
    EXPECT_EQ(token.size(), 0);
}

TEST(NumberTokenFactory, canHandleMultipleNumbers) {
    std::wstring data = L"123 456";
    auto stream = std::wistringstream(data);
    auto tokenizer = nugdev::compiler::tokenize::Tokenizer({std::make_shared<nugdev::compiler::tokenize::NumberTokenFactory>()});
    auto token = tokenizer.tokenize(stream);
    EXPECT_EQ(token.size(), 2);
    EXPECT_EQ(token[0].to_str(), icu::UnicodeString::fromUTF8("123"));
    EXPECT_EQ(token[1].to_str(), icu::UnicodeString::fromUTF8("456"));
}
