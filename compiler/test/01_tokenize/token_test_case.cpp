#include "token_test_case.h"

void TokenTestCase::SetUp() {}

bool TokenTestCase::load_sample(const std::string_view &sampleName) {
    codeFile.open(sampleFilePaths.at(sampleName).first);
    tokenFile.open(sampleFilePaths.at(sampleName).second);

    if (!codeFile.is_open() || !tokenFile.is_open()) {
        return false;
    }

    return true;
}

std::vector<icu::UnicodeString> TokenTestCase::get_code_lines() {
    std::vector<icu::UnicodeString> lines;
    std::string line;
    while (std::getline(codeFile, line)) {
        lines.push_back(icu::UnicodeString::fromUTF8(line));
    }
    return lines;
}

std::vector<icu::UnicodeString> TokenTestCase::get_token_lines() {
    std::vector<icu::UnicodeString> lines;
    std::string line;
    while (std::getline(tokenFile, line)) {
        lines.push_back(icu::UnicodeString::fromUTF8(line));
    }
    return lines;
}