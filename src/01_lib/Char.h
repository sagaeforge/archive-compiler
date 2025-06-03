#pragma once

#include <unicode/unistr.h>

namespace nugdev::lib {

class Char {
public:
  using char_t = UChar;
  using byte_t = std::uint8_t;
  using self_t = Char;

public:
  Char(const char_t ch);

public:
  operator char_t() const;

public:
  bool isDigit() const;
  bool isAlpha() const;
  bool isUpper() const;
  bool isLower() const;
  bool isAlnum() const;
  bool isSpace() const;

  byte_t to_digit() const;
  char_t to_char() const;
  char_t get_char() const;
  self_t to_upper() const;
  self_t to_lower() const;
  self_t set_char(const char_t ch) const;

private:
  char_t ch;
};
} // namespace nugdev::lib