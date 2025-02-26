#include "parser_test_case.h"

#include <filesystem>
#include <fstream>

#include "00_app/json/Json.hpp"
#include <rapidjson/prettywriter.h>

void ParserTestCase::SetUp() {
    tokenizer = Tokenizer();
    parser = Parser();
}

bool ParserTestCase::load_sample(const std::string_view &testCaseName, const std::string_view &testCasePath) {
    auto parentPath = std::filesystem::path(testCasePath).parent_path();
    auto expectedFilePath = parentPath / (std::string(testCaseName) + ".json");
    std::ifstream expectedFile(expectedFilePath);
    if (!expectedFile.is_open()) {
        return false;
    }

    std::string json_content;
    std::string line;
    while (std::getline(expectedFile, line)) {
        json_content += line;
    }

    if (answer.Parse(json_content.c_str()).HasParseError()) {
        return false;
    }
    return true;
}

void ParserTestCase::expect_ast(const JsonDocument &ast) {
    rapidjson::StringBuffer expected_buffer;
    rapidjson::Writer<rapidjson::StringBuffer> expected_writer(expected_buffer);
    ast.Accept(expected_writer);

    rapidjson::StringBuffer actual_buffer;
    rapidjson::Writer<rapidjson::StringBuffer> actual_writer(actual_buffer);
    answer.Accept(actual_writer);

    std::string expected_str(expected_buffer.GetString());
    std::string actual_str(actual_buffer.GetString());

    EXPECT_EQ(expected_str, actual_str);
}
