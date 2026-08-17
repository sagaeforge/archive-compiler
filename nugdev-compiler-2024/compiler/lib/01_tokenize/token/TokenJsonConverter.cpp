#include "01_tokenize/token/TokenJsonConverter.h"

#include "00_lib/lib/Json.hpp"
#include "00_lib/lib/String.h"

namespace nugdev::compiler::tokenize {

std::optional<lib::JsonValue> TokenJsonConverter::serialize(const Token &token, lib::JsonAllocator &allocator) {
    lib::JsonValue value(lib::Type::kObjectType);
    value.AddMember("type", lib::create_json_value(lib::to_string(token.get_type()), allocator), allocator);
    value.AddMember("literal", lib::create_json_value(token.get_literal(), allocator), allocator);

    return value;
}

std::optional<lib::JsonValue> TokenJsonConverter::serialize(const std::vector<Token> &tokens, lib::JsonAllocator &allocator) {
    lib::JsonValue values(lib::Type::kArrayType);
    for (const auto &token : tokens) {
        values.PushBack(serialize(token, allocator).value(), allocator);
    }
    return values;
}

std::optional<Token> TokenJsonConverter::deserialize(const lib::JsonValue &value) {
    auto type = lib::to_enum<TokenType>(value["type"].GetString());
    if (!type) {
        return std::nullopt;
    }
    auto literal = value["literal"].GetString();
    return Token(type.value(), literal);
}

std::optional<std::vector<Token>> TokenJsonConverter::deserialize(const lib::JsonArray &values) {
    std::vector<Token> tokens;
    for (const auto &value : values) {
        tokens.push_back(deserialize(value).value());
    }
    return tokens;
}

}  // namespace nugdev::compiler::tokenize
