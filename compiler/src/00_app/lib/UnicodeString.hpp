#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <unicode/unistr.h>

namespace std {
// ICU UnicodeString에 대한 해시 함수 특수화
template <> struct hash<icu::UnicodeString> {
    size_t operator()(const icu::UnicodeString &str) const {
        // UTF-8로 변환하여 std::string의 해시 함수를 재사용
        std::string utf8;
        str.toUTF8String(utf8);
        return std::hash<std::string>{}(utf8);
    }
};

} // namespace std

namespace nugdev::compiler::lib {

struct String : public icu::UnicodeString {
    using icu::UnicodeString::UnicodeString;

    String(const icu::UnicodeString &str) : icu::UnicodeString(str) {}

    std::string to_string() const {
        std::string utf8;
        this->toUTF8String(utf8);
        return utf8;
    }

    double to_double() const {
        std::string utf8;
        this->toUTF8String(utf8);
        return std::stod(utf8);
    }

    std::int64_t to_int64() const {
        std::string utf8;
        this->toUTF8String(utf8);
        return std::stoll(utf8);
    }

    size_t get_buffer_size() const { return to_string().size(); }

    bool has_decimal_point() const { return this->indexOf('.') != std::string::npos; }
};

} // namespace nugdev::compiler::lib