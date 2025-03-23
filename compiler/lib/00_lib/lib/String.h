#pragma once

#define MAGIC_ENUM_RANGE_MIN 0
#define MAGIC_ENUM_RANGE_MAX 256
#include <unicode/unistr.h>

#include <magic_enum/magic_enum.hpp>
#include <string_view>

namespace nugdev::compiler::lib {

using Char = char16_t;
class String : public icu::UnicodeString {
public:
    using icu::UnicodeString::UnicodeString;
    using iterator_t = std::uint32_t;
    using char_t = Char;

public:
    String(const char_t ch);
    String(const std::string &str);
    String(const std::wstring &str);
    String(const std::string_view &str);
    String(const icu::UnicodeString &str);
    String(const std::vector<String> &strs, const String &delimiter);

public:  // 문자열 변환
    std::string to_string() const;
    std::vector<char_t> to_vector() const;

public:  // 기타 함수
    String slice(const iterator_t &start, const iterator_t &end) const;
    std::vector<String> split(const String &delimiter) const;
};

template <typename Type>
    requires std::is_arithmetic_v<Type>
String to_string(const Type &value) {
    return String(std::to_string(value));
}

template <typename Type>
    requires std::is_enum_v<Type>
String to_string(const Type &value) {
    auto str = magic_enum::enum_name(value);
    return String(str.data());
}

template <typename Type>
    requires std::is_enum_v<Type>
std::optional<Type> to_enum(const String &str) {
    return magic_enum::enum_cast<Type>(str.to_string());
}

}  // namespace nugdev::compiler::lib

namespace std {
template <>
struct hash<nugdev::compiler::lib::String> {
    size_t operator()(const nugdev::compiler::lib::String &str) const {
        return std::hash<std::string>{}(str.to_string());
    }
};

}  // namespace std