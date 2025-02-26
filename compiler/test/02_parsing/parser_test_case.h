#pragma once

#include <gtest/gtest.h>

#include <fstream>
#include <string_view>
#include <unicode/unistr.h>

#include "00_app/json/Json.hpp"
#include "01_tokenize/Tokenizer.h"
#include "02_parsing/Parser.h"

using namespace nugdev::compiler::tokenize;
using namespace nugdev::compiler::parsing;
using namespace nugdev::compiler::json;

class ParserTestCase : public ::testing::Test {
  protected:
    void SetUp() override;

  protected:
    bool load_sample(const std::string_view &testCaseName, const std::string_view &testCasePath);
    void expect_ast(const JsonDocument &ast);

  protected:
    JsonDocument answer;
    Tokenizer tokenizer;
    Parser parser;
};

#define LOAD_SAMPLE(testCaseName) load_sample(#testCaseName, __FILE__)
