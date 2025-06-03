#include "Char.h"

namespace nugdev::lib {

Char::Char(const char_t ch) : ch(ch) {}

Char::operator char_t() const { return ch; }

bool Char::isDigit() const { return '0' <= ch && ch <= '9'; }
bool Char::isAlpha() const {
  return ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z');
}
bool Char::isUpper() const { return 'A' <= ch && ch <= 'Z'; }
bool Char::isLower() const { return 'a' <= ch && ch <= 'z'; }
bool Char::isAlnum() const { return isDigit() || isAlpha(); }
bool Char::isSpace() const {
  return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == '\f' ||
         ch == '\v';
}

Char::byte_t Char::to_digit() const { return ch - '0'; }
Char::char_t Char::to_char() const { return ch; }
Char::char_t Char::get_char() const { return ch; }
Char Char::to_upper() const { return isUpper() ? *this : Char(ch - 'a' + 'A'); }
Char Char::to_lower() const { return isLower() ? *this : Char(ch - 'A' + 'a'); }
Char Char::set_char(const char_t ch) const { return Char(ch); }

} // namespace nugdev::lib