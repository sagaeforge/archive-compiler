#include "IdentifierToken.h"

namespace nugdev::compiler::tokenize {

IdentifierToken::IdentifierToken(const icu::UnicodeString &value) : value(value) {}

icu::UnicodeString IdentifierToken::to_str() { return this->value; }

} // namespace nugdev::compiler::tokenize
