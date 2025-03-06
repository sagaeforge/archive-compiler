#pragma once

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