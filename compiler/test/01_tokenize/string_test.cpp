#include <gtest/gtest.h>
#include <unicode/unistr.h>

#include "01_tokenize/Tokenizer.h"
#include "01_tokenize/factory/StringTokenFactory.h"

TEST(StringTokenFactory, canHandle) {
    std::wstring data = L"\"test\"";
    auto stream = std::wistringstream(data);
    auto tokenizer = nugdev::compiler::tokenize::Tokenizer({std::make_shared<nugdev::compiler::tokenize::StringTokenFactory>()});
    auto token = tokenizer.tokenize(stream);
    EXPECT_EQ(token.size(), 1);
    EXPECT_EQ(token[0].to_str(), icu::UnicodeString::fromUTF8("test"));
}

TEST(StringTokenFactory, canNotHandle) {
    std::wstring data = L"test";
    auto stream = std::wistringstream(data);
    auto tokenizer = nugdev::compiler::tokenize::Tokenizer({std::make_shared<nugdev::compiler::tokenize::StringTokenFactory>()});
    auto token = tokenizer.tokenize(stream);
    EXPECT_EQ(token.size(), 0);
}

TEST(StringTokenFactory, canHandleMultipleQuotes) {
    std::wstring data = L"\"test\"\"test\"";
    auto stream = std::wistringstream(data);
    auto tokenizer = nugdev::compiler::tokenize::Tokenizer({std::make_shared<nugdev::compiler::tokenize::StringTokenFactory>()});
}