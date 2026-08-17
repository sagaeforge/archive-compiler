//
// Created by lambda on 10/31/25.
//

#pragma once
#include "token/token.h"

class TokenConverter final {
public:
    [[nodiscard]] static Token importJson(nlohmann::json &json);

    [[nodiscard]] static nlohmann::json exportJson(const Token &token);
};
