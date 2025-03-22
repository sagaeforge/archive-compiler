#pragma once

#include "00_lib/lib/Json.hpp"
#include "01_tokenize/Token.h"

namespace nugdev::compiler::tokenize {

class TokenJsonConverter : public lib::JsonSerializer<Token>, public lib::JsonDeserializer<Token> {
  public:
    std::optional<lib::JsonValue> serialize(const Token &token, lib::JsonAllocator &allocator);
    std::optional<lib::JsonValue> serialize(const std::vector<Token> &tokens, lib::JsonAllocator &allocator);
    std::optional<Token> deserialize(const lib::JsonValue &value);
    std::optional<std::vector<Token>> deserialize(const lib::JsonArray &values);
};

} // namespace nugdev::compiler::tokenize
