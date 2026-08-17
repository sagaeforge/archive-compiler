//
// Created by lambda on 10/31/25.
//

#include "token_converter.h"

#include "00_core/magic_enum/magic_enum.hpp"

Token TokenConverter::importJson(nlohmann::json &json) {
    try {
        const string_t literal = json.at("literal");
        const std::string typeStr = json.at("type");
        const auto type = magic_enum::enum_cast<TokenType>(typeStr).value_or(TokenType::Illegal);
        const string_t &fileName = json.at("file-name");
        const int &line = json.at("file-line");
        const int &column = json.at("file-column");
        return {literal, type, fileName, line, column};
    } catch (const nlohmann::json::out_of_range &e) {
        // 필드가 없는 경우 처리
        throw std::invalid_argument("Invalid token JSON: missing required fields");
    }
}

nlohmann::json TokenConverter::exportJson(const Token &token) {
    nlohmann::json json;
    json["literal"] = token.literal();
    json["type"] = magic_enum::enum_name(token.type());
    json["file-name"] = token.fileName();
    json["file-line"] = token.line();
    json["file-column"] = token.column();
    return json;
}
