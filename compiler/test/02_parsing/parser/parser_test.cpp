#include "gtest/gtest.h"

#include "00_app/json/Json.hpp"
#include "01_tokenize/Tokenizer.h"
#include "02_parsing/Parser.h"

namespace nugdev::compiler::test {

TEST(ParserTest, ParseProgram) {
    auto code = u"let x = 10";
    auto tokenizer = tokenize::Tokenizer();
    auto tokens = tokenizer.tokenize(code);
    auto parser = parsing::Parser();
    auto module = parser.parse(tokens);

    json::JsonDocument document;
    auto json = module->to_json(document.GetAllocator());
    document.SetObject().AddMember("module", json, document.GetAllocator());
}

} // namespace nugdev::compiler::test
