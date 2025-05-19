#include "String.h"

namespace nugdev::lib {

String::String(const char_t ch) : icu::UnicodeString(ch) {}

String::String(const std::string &str) : icu::UnicodeString(str.c_str()) {}

String::String(const std::wstring &str) : icu::UnicodeString() {
  for (auto ch : str) {
    this->append(ch);
  }
}

String::String(const std::string_view &str) : icu::UnicodeString(str.data()) {}

String::String(const icu::UnicodeString &str) : icu::UnicodeString(str) {}

String::String(const std::vector<String> &strs, const String &delimiter)
    : icu::UnicodeString() {
  for (auto i = 0; i < static_cast<int>(strs.size()); i++) {
    this->append(strs[i]);
    if (i < static_cast<int>(strs.size()) - 1) {
      this->append(delimiter);
    }
  }
}

std::string String::to_string() const {
  std::string utf8;
  this->toUTF8String(utf8);
  return utf8;
}

std::vector<String::char_t> String::to_vector() const {
  std::vector<String::char_t> result;
  result.reserve(this->length());
  for (auto i = 0; i < this->length(); i++) {
    result.push_back(this->charAt(i));
  }
  return result;
}

String String::slice(const iterator_t &start, const iterator_t &end) const {
  return String(this->tempSubString(start, end - start));
}

std::vector<String> String::split(const String &delimiter) const {
  std::vector<String> result;
  auto pos = 0;
  auto delim_pos = this->indexOf(delimiter);
  while (delim_pos != -1) {
    result.push_back(this->slice(pos, delim_pos));
    pos = delim_pos + delimiter.length();
    delim_pos = this->indexOf(delimiter, pos);
  }
  result.push_back(this->slice(pos, this->length()));
  return result;
}

} // namespace nugdev::lib