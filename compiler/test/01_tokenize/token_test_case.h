#pragma once

#include <gtest/gtest.h>

#include <fstream>
#include <string_view>
#include <unicode/unistr.h>
#include <unordered_map>

class TokenTestCase : public ::testing::Test {
  protected:
    void SetUp() override;

  protected:
    bool load_sample(const std::string_view &sampleName);
    std::vector<icu::UnicodeString> get_code_lines();
    std::vector<icu::UnicodeString> get_token_lines();

  private:
    std::ifstream codeFile;
    std::ifstream tokenFile;

  protected:
    // 헤더 파일에서
    static inline std::unordered_map<std::string_view, std::pair<std::string_view, std::string_view>> sampleFilePaths = {
        {"init_token",
         {"/Users/nugdev-book/Projects/compiler/compiler/test/01_tokenize/01_init_token/sample.code",
          "/Users/nugdev-book/Projects/compiler/compiler/test/01_tokenize/01_init_token/sample.token"}},
    };
};
