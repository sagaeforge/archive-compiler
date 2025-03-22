#include "TestBase.h"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>

#include "00_lib/lib/Json.hpp"
#include "lib/00_lib/lib/String.h"

namespace nugdev::compiler::test {

void TestBase::setup() {}

void TestBase::teardown() {}

void TestBase::SetUp() {
    m_expacted_values = lib::String();
    setup();
    std::cout << "-------------------------------- nugdev compiler test --------------------------------" << std::endl;
    std::cout << "test name: " << get_test_info()->name() << std::endl;
    std::cout << "1. load expacted file... ";
    bool is_loaded = load_expacted_file();
    std::string result = is_loaded ? "success" : "not found";
    std::cout << result << std::endl;
    std::cout << "-------------------------------- nugdev compiler test --------------------------------" << std::endl;
}

void TestBase::TearDown() { teardown(); }

const testing::TestInfo *TestBase::get_test_info() { return ::testing::UnitTest::GetInstance()->current_test_info(); }

void TestBase::set_expacted_file_extension(const lib::String &extension) { m_expacted_file_extension = extension; }

bool TestBase::load_expacted_file() {
    auto test_info = get_test_info();
    lib::String test_name = test_info->name();

    // 현재 테스트 케이스가 포함된 경로를 반환함.
    std::filesystem::path file_path = test_info->file();
    auto dir = file_path.parent_path();

    auto expected_file_path = dir / lib::String(test_name + u"." + m_expacted_file_extension).to_string();

    // 이제, 파일 까서 봄
    std::wifstream file(expected_file_path.wstring());
    if (!file.is_open()) {
        // 파일이 없는 경우, 밑에 코드를 실행할 필요가 없음.
        return false;
    }

    std::wstring line;
    while (std::getline(file, line)) {
        m_expacted_values += lib::String(line);
    }

    return true;
}

void TestBase::expected_result(const lib::String &value) {
    if (m_expacted_values.isEmpty()) {
        ADD_FAILURE() << "expacted values is empty";
    }

    ASSERT_EQ(m_expacted_values, value);
}

void TestBase::expected_result(const lib::JsonDocument &document) {
    // 문자열로 변환한 다음. 비교.
    lib::JsonDocument answer;
    if (answer.Parse(m_expacted_values.to_string().c_str()).HasParseError()) {
        ADD_FAILURE() << "expacted values is not a valid json";
    }

    // 두 개다 문자열로 변환한 다음. 비교.
    lib::JsonStringBuffer actualBuffer;
    lib::JsonWriter actualWriter(actualBuffer);
    document.Accept(actualWriter);
    std::string actualStr(actualBuffer.GetString());

    lib::JsonStringBuffer expectedBuffer;
    lib::JsonWriter expectedWriter(expectedBuffer);
    answer.Accept(expectedWriter);
    std::string expectedStr(expectedBuffer.GetString());

    ASSERT_EQ(actualStr, expectedStr);
}

void TestBase::expected_result(const lib::JsonValue &value) {
    // 문자열로 변환한 다음. 비교.
    lib::JsonDocument answer;
    if (answer.Parse(m_expacted_values.to_string().c_str()).HasParseError()) {
        ADD_FAILURE() << "expacted values is not a valid json";
    }
    lib::JsonStringBuffer actualBuffer;
    lib::JsonWriter actualWriter(actualBuffer);
    value.Accept(actualWriter);
    std::string actualStr(actualBuffer.GetString());

    lib::JsonStringBuffer expectedBuffer;
    lib::JsonWriter expectedWriter(expectedBuffer);
    answer.Accept(expectedWriter);
    std::string expectedStr(expectedBuffer.GetString());

    ASSERT_EQ(actualStr, expectedStr);
}

} // namespace nugdev::compiler::test