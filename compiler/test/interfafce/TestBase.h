#pragma once

#include <gtest/gtest.h>

#include "lib/00_lib/lib/Json.hpp"
#include "lib/00_lib/lib/String.h"

namespace nugdev::compiler::test {

class TestBase : public ::testing::Test {
protected:
    virtual void setup();
    virtual void teardown();

protected:
    void set_expacted_file_extension(const lib::String &extension);
    void expected_result(const lib::String &value);
    void expected_result(const lib::JsonDocument &document);
    void expected_result(const lib::JsonValue &value);

private:
    void SetUp() override;
    void TearDown() override;

private:
    bool load_expacted_file();
    const testing::TestInfo *get_test_info();

private:
    lib::String m_expacted_file_extension = u"json";
    lib::String m_expacted_values;
    lib::JsonDocument m_document;
};

}  // namespace nugdev::compiler::test
