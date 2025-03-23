#include "00_lib/lib/Json.hpp"
#include "01_tokenize/Tokenizer.h"
#include "01_tokenize/token/TokenJsonConverter.h"
#include "test/01_tokenize/TokenTestFixture.h"

namespace nugdev::compiler::test {

TEST_F(TokenTestFixture, var_define) {
    auto source = lib::String(u"let a = 1");
    auto tokenizer = tokenize::Tokenizer();
    auto tokens = tokenizer.tokenize(source);

    auto document = lib::JsonDocument();
    auto converter = tokenize::TokenJsonConverter();
    auto json = converter.serialize(tokens, document.GetAllocator());

    expected_result(json.value());
}

TEST_F(TokenTestFixture, comment) {
    auto source = lib::String(u"# comment");
    auto tokenizer = tokenize::Tokenizer();
    auto tokens = tokenizer.tokenize(source);

    EXPECT_EQ(tokens.size(), 0);
}
}  // namespace nugdev::compiler::test
