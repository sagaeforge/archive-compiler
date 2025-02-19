#include "gtest/gtest.h"

#include <fstream>
#include <string_view>
#include <unordered_map>

class TokenTestCase : public ::testing::Test {
  protected:
    void SetUp() override;

  protected:
    bool loadSample(const std::string_view &sampleName);

  private:
    std::ifstream codeFile;
    std::ifstream tokenFile;

  protected:
    // 헤더 파일에서
    static inline std::unordered_map<std::string_view, std::pair<std::string_view, std::string_view>> sampleFilePaths = {
        {"sample", {"./sample.code", "./sample.token"}},
        {"test", {"./test.code", "./test.token"}},
    };
};
