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

    String(const std::string &str) : icu::UnicodeString(str.c_str()) {}
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

    String slice(const size_t &start, const size_t &end) const {
        // 범위 유효성 검사
        if (start > end) {
            return String();
        }

        if (end > this->length()) {
            return this->tempSubString(start, this->length() - start);
        }

        if (start >= this->length()) {
            return String();
        }

        // ICU UnicodeString의 tempSubString 메서드 활용
        return String(this->tempSubString(start, end - start));
    }

    // 문자열을 구분자로 분할하여 벡터로 반환
    // limit: 최대 분할 횟수 (기본값 -1은 제한 없음)
    std::vector<String> split(const String &delimiter, int limit = -1) const {
        // 빈 문자열 처리
        if (this->isEmpty()) {
            return {String()};
        }

        // 빈 구분자 처리 (한 글자씩 분할)
        if (delimiter.isEmpty()) {
            std::vector<String> result;
            for (int32_t i = 0; i < this->length(); ++i) {
                UChar32 ch = this->char32At(i);
                result.push_back(String().append(ch));
            }
            return result;
        }

        auto copy = *this;
        std::vector<String> result;
        int count = 0;

        while (limit < 0 || count < limit - 1) {
            auto pos = copy.indexOf(delimiter);
            if (pos == -1) {
                break;
            }

            result.push_back(copy.slice(0, pos));
            copy = copy.slice(pos + delimiter.length(), copy.length());
            count++;
        }

        // 남은 부분 추가
        result.push_back(copy);

        return result;
    }
};

} // namespace nugdev::compiler::lib