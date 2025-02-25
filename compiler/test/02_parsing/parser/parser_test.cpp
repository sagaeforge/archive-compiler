#include "gtest/gtest.h"

#include <fstream>

#include "rapidjson/prettywriter.h"

#include "00_app/json/Json.hpp"
#include "01_tokenize/Tokenizer.h"
#include "02_parsing/Parser.h"

namespace nugdev::compiler::test {

TEST(ParserTest, ParseProgram) {
    auto code = u"if 10 == 10 { \"abc\" } elif 10 == 20 { \"def\" } else { false }";
    auto tokenizer = tokenize::Tokenizer();
    auto tokens = tokenizer.tokenize(code);
    auto parser = parsing::Parser();
    auto module = parser.parse(tokens);

    json::JsonDocument document;
    auto json = module->to_json(document.GetAllocator());
    document.SetObject().AddMember("module", json, document.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);

    // ./ast.json 파일에 저장
    std::ofstream file("ast.json");
    file << buffer.GetString();
    file.close();
}

} // namespace nugdev::compiler::test
