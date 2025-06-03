#pragma once

#include <01_lib/Json.hpp>
#include <03_tokenize/Token.h>

namespace nugdev::compiler::tokenize {

class TokenConverter {
public:
  TokenConverter();
  ~TokenConverter();

public:
  lib::JsonValue to_json(const Token &token) const;
  Token from_json(const lib::JsonValue &json) const;

private:
  class JsonConverter : public lib::JsonSerializable<Token> {
  public:
    std::optional<lib::JsonValue> serialize(const Token &source) const override;
    std::optional<Token>
    deserialize(const lib::JsonValue &target) const override;
  };

private:
  JsonConverter jsonConverter;
};

} // namespace nugdev::compiler::tokenize
