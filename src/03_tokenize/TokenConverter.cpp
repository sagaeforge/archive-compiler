#include "TokenConverter.h"

namespace nugdev::compiler::tokenize {

TokenConverter::TokenConverter() = default;

TokenConverter::~TokenConverter() = default;

lib::JsonValue TokenConverter::to_json(const Token &token) const {
  auto result = jsonConverter.serialize(token);
  if (result.has_value()) {
    return std::move(result.value());
  }
  // Return empty JSON object if serialization fails
  lib::JsonValue emptyObject;
  emptyObject.SetObject();
  return emptyObject;
}

Token TokenConverter::from_json(const lib::JsonValue &json) const {
  auto result = jsonConverter.deserialize(json);
  if (result.has_value()) {
    return result.value();
  }
  // Return Illegal Token if deserialization fails
  return Token(TokenType::Illegal, lib::String(""));
}

std::optional<lib::JsonValue>
TokenConverter::JsonConverter::serialize(const Token &source) const {
  static lib::JsonDocument doc;
  doc.SetObject();
  auto &allocator = doc.GetAllocator();

  lib::JsonValue json(rapidjson::kObjectType);

  // Add type field using magic_enum string conversion
  lib::String typeStr = lib::to_string(source.get_type());
  auto typeStdStr = typeStr.to_string();
  json.AddMember(
      "type", lib::JsonValue(typeStdStr.c_str(), typeStdStr.size(), allocator),
      allocator);

  // Add literal field
  auto literal = source.get_literal().to_string();
  json.AddMember("literal",
                 lib::JsonValue(literal.c_str(), literal.size(), allocator),
                 allocator);

  return json;
}

std::optional<Token>
TokenConverter::JsonConverter::deserialize(const lib::JsonValue &target) const {
  if (!target.IsObject()) {
    return std::nullopt;
  }

  if (!target.HasMember("type") || !target.HasMember("literal")) {
    return std::nullopt;
  }

  if (!target["type"].IsString() || !target["literal"].IsString()) {
    return std::nullopt;
  }

  // Convert string type back to enum using magic_enum
  std::string typeStr = target["type"].GetString();
  lib::String libTypeStr(typeStr);
  auto typeOpt = lib::to_enum<TokenType>(libTypeStr);
  if (!typeOpt.has_value()) {
    return std::nullopt;
  }

  TokenType type = typeOpt.value();
  std::string literal = target["literal"].GetString();

  return Token(type, lib::String(literal));
}

} // namespace nugdev::compiler::tokenize