#include "parser_test_case.h"

#include <filesystem>
#include <fstream>

#include "00_app/json/Json.hpp"
#include "02_parsing/ast/AST.h"
#include "02_parsing/ast/visitor/ASTNodeJsonVisitor.h"
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

void ParserTestCase::expect_ast(const nugdev::compiler::ast::ASTNodePtr &ast) {
    auto visitor = std::make_shared<nugdev::compiler::ast::ASTNodeJsonVisitor>();
    auto expected = ast->accept<nugdev::compiler::json::JsonValue>(visitor);
    auto expectedStr = visitor->to_str(expected).to_string();

    nugdev::compiler::json::JsonStringBuffer actualBuffer;
    nugdev::compiler::json::JsonFormatter actualWriter(actualBuffer);
    answer.Accept(actualWriter);
    std::string actualStr(actualBuffer.GetString());

    EXPECT_EQ(expectedStr, actualStr);
}
